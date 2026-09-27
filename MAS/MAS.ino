/*
 * =====================================================================================
 * ESP32 MASTER: Dual I2C, PZEM Master, NVS, Daily Energy Loss & Auto-NTP Reset at 10 PM
 * =====================================================================================
 */

#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PZEM004Tv30.h>
#include <Preferences.h>
#include <time.h> // Include time library for NTP

//======================================================================================
// CONFIGURATION
//======================================================================================
/* WiFi Credentials */
#define WIFI_SSID     "RESEARCH LAB"
#define WIFI_PASSWORD "G1234567890"

/* Firebase Credentials */
#define API_KEY       "AIzaSyBZLtf0wO0jivZXzfebOIehXAXuF40KCCA"
#define DATABASE_URL  "https://esp32-918b2-default-rtdb.firebaseio.com/"
#define USER_EMAIL    "esp32@gmail.com"
#define USER_PASS     "emmy4012"

/* Master PZEM Pins */
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17

/* Dual I2C Configuration */
#define SLAVE_SDA 21
#define SLAVE_SCL 22
#define HOUSE1_ADDRESS 0x08
#define HOUSE2_ADDRESS 0x09

#define LCD_SDA 25
#define LCD_SCL 26
#define LCD_ADDRESS 0x27

/* Theft Detection Tolerance (in PERCENTAGE - e.g., 5.0 = 5%) */
const float THEFT_TOLERANCE_PERCENT = 5.0; 

/* NTP Time Configuration (Nigeria is UTC+1, no daylight saving) */
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;      // 1 hour = 3600 seconds
const int   daylightOffset_sec = 0;

//======================================================================================
// DATA STRUCTURES (Matches Slave struct)
//======================================================================================
struct HouseData {
    float energy;   // kWh
    float current;  // Amperes
};

//======================================================================================
// GLOBAL FIREBASE PATHS
//======================================================================================
String theftAlertsPath = "master/theft_alerts";
String gridStatusPath  = "master/status";
String commandPath     = "master/command";
String masterLivePath  = "master/live_metrics";

//======================================================================================
// GLOBAL OBJECTS
//======================================================================================
/* I2C Buses */
TwoWire SlaveBus = TwoWire(1); // Controller 1 for Slaves
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2); // LCD on Controller 0 (Wire)

/* Master PZEM */
HardwareSerial pzemSerial(2);
PZEM004Tv30 pzemMaster(pzemSerial, PZEM_RX_PIN, PZEM_TX_PIN);

/* NVS Preferences Storage */
Preferences prefs;

/* Firebase */
UserAuth userAuth(API_KEY, USER_EMAIL, USER_PASS);
WiFiClientSecure sslClient;
FirebaseApp app;
RealtimeDatabase Database;
using AsyncClient = AsyncClientClass;
AsyncClient client(sslClient);

//======================================================================================
// VARIABLES
//======================================================================================
float masterEnergy = 0.0;
float masterCurrent = 0.0; 

HouseData house1 = {0.0f, 0.0f};
HouseData house2 = {0.0f, 0.0f};

// NVS Baseline & Daily Metrics
float refMasterEnergy = 0.0;
float refHouse1Energy = 0.0;
float refHouse2Energy = 0.0;

float dailyMasterUsage = 0.0;
float dailyHouse1Usage = 0.0;
float dailyHouse2Usage = 0.0;
float totalHouseEnergy = 0.0;
float dailyEnergyLoss  = 0.0;
float lossPercentage   = 0.0;

bool isTheftDetected = false;

unsigned long lastReadTime = 0;
unsigned long lastAlertTime = 0;
unsigned long lastCommandCheckTime = 0;
unsigned long lastFirebaseUploadTime = 0; // <-- NEW TIMER FOR LIVE UPLOAD
int lastResetDay = -1; // Tracks day of month to prevent multiple resets in the same hour
// --- NEW THEFT TIMER VARIABLES ---
unsigned long theftConditionStartTime = 0;            
const unsigned long THEFT_CONFIRMATION_DELAY = 10000; // 10 seconds delay
//======================================================================================
// FUNCTION PROTOTYPES
//======================================================================================
void connectWiFi();
void setupFirebase();
void authHandler(AsyncResult &result);
void fetchDailyEnergies();
HouseData readSlaveI2C(uint8_t address);
void updateLCD();
void handleTheftAlert();
void loadNVSReferences();
void resetDailyBaselines();
void checkFirebaseCommands();
void checkAutoResetTime();
void uploadMasterLiveMetrics();

//======================================================================================
// SETUP
//======================================================================================
void setup() {
    Serial.begin(115200);

    // 1. START LCD I2C (Controller 0)
    Wire.begin(LCD_SDA, LCD_SCL, 100000);
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MASTER BOOTING");

    // 2. START SLAVE I2C (Controller 1)
    SlaveBus.begin(SLAVE_SDA, SLAVE_SCL, 100000);

    // 3. Initialize Master PZEM
    pzemSerial.begin(9600, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);

    // 4. Initialize NVS Storage and Load Reference Baselines
    prefs.begin("energy_nvs", false);
    loadNVSReferences();

    // 5. Network, NTP & Cloud Setup
    connectWiFi();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // Initialize NTP sync
    setupFirebase();

    lcd.clear();
    lcd.print("SYSTEM READY");
    delay(2000);
}

// //======================================================================================
// // LOOP
// //======================================================================================
// void loop() {
//     app.loop();
//     unsigned long now = millis();

//     // Run the auditing cycle every 3 seconds
//     if (now - lastReadTime >= 3000) {
//         lastReadTime = now;
        
//         fetchDailyEnergies();
//         updateLCD();
//         handleTheftAlert();
//         checkAutoResetTime(); // Check if it's 10:00 PM for automated reset
//         // <-- NEW CALL: Uploads Live Metrics to Firebase every 3 seconds
//         uploadMasterLiveMetrics();
//     }

//     // Check Firebase for remote commands every 10 seconds
//     if (now - lastCommandCheckTime >= 10000) {
//         lastCommandCheckTime = now;
//         checkFirebaseCommands();
//     }
// }


//======================================================================================
// LOOP
//======================================================================================
void loop() {
    app.loop();
    unsigned long now = millis();

    // 1. FAST LOCAL CYCLE: Read sensors, check theft, and update LCD every 2 seconds
    if (now - lastReadTime >=3000) {
        lastReadTime = now;
        
        fetchDailyEnergies();
        updateLCD();
        handleTheftAlert();
        checkAutoResetTime(); // Check if it's 10:00 PM for automated reset
    }

    // 2. CLOUD METRICS CYCLE: Upload Live Metrics to Firebase every 5 seconds
    if (now - lastFirebaseUploadTime >= 5000) {
        lastFirebaseUploadTime = now;
        
        uploadMasterLiveMetrics();
    }

    // 3. CLOUD COMMAND CYCLE: Check Firebase for remote commands every 10 seconds
    if (now - lastCommandCheckTime >= 10000) {
        lastCommandCheckTime = now;
        checkFirebaseCommands();
    }
}

//======================================================================================
// NVS STORAGE MANAGEMENT (Baselines & Daily Calculations)
//======================================================================================
void loadNVSReferences() {
    refMasterEnergy = prefs.getFloat("ref_master", -1.0);
    refHouse1Energy = prefs.getFloat("ref_h1", -1.0);
    refHouse2Energy = prefs.getFloat("ref_h2", -1.0);

    float initialMaster = pzemMaster.energy();
    if (isnan(initialMaster)) initialMaster = 0.0;
    
    HouseData initH1 = readSlaveI2C(HOUSE1_ADDRESS);
    HouseData initH2 = readSlaveI2C(HOUSE2_ADDRESS);

    if (refMasterEnergy < 0.0) {
        refMasterEnergy = initialMaster;
        prefs.putFloat("ref_master", refMasterEnergy);
    }
    if (refHouse1Energy < 0.0) {
        refHouse1Energy = initH1.energy;
        prefs.putFloat("ref_h1", refHouse1Energy);
    }
    if (refHouse2Energy < 0.0) {
        refHouse2Energy = initH2.energy;
        prefs.putFloat("ref_h2", refHouse2Energy);
    }
}

void resetDailyBaselines() {
    refMasterEnergy = masterEnergy;
    refHouse1Energy = house1.energy;
    refHouse2Energy = house2.energy;

    prefs.putFloat("ref_master", refMasterEnergy);
    prefs.putFloat("ref_h1", refHouse1Energy);
    prefs.putFloat("ref_h2", refHouse2Energy);
    
    Serial.println(">> NVS Daily Baselines Reset Successfully! <<");
}

// Automatic check for 10:00 PM (22:00) using NTP time
void checkAutoResetTime() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        // Check if hour is 22 (10 PM) and minute is 0, and we haven't already reset today
        if (timeinfo.tm_hour == 22 && timeinfo.tm_min == 0 && timeinfo.tm_mday != lastResetDay) {
            Serial.println(">> 10:00 PM Reached: Triggering Automatic Daily Reset... <<");
            resetDailyBaselines();
            lastResetDay = timeinfo.tm_mday; // Lock out until tomorrow
        }
    }
}

//======================================================================================
// FETCH DATA & DAILY ENERGY LOSS COMPUTATION
//======================================================================================
void fetchDailyEnergies() {
    // 1. Read Master Energy & Current (Main Grid)
    float mE = pzemMaster.energy();
    if (!isnan(mE)) {
        masterEnergy = mE;
    }
    
    float mI = pzemMaster.current();
    if (!isnan(mI)) {
        masterCurrent = mI;
    } else {
        masterCurrent = 0.0;
    }

    // 2. Request Data (Energy + Current struct) from House 1 and House 2
    house1 = readSlaveI2C(HOUSE1_ADDRESS);
    delay(50); 
    house2 = readSlaveI2C(HOUSE2_ADDRESS);

    // 3. Calculate Daily Net Usage per Meter using NVS Reference Subtraction
    dailyMasterUsage = (masterEnergy >= refMasterEnergy) ? (masterEnergy - refMasterEnergy) : 0.0;
    dailyHouse1Usage = (house1.energy >= refHouse1Energy) ? (house1.energy - refHouse1Energy) : 0.0;
    dailyHouse2Usage = (house2.energy >= refHouse2Energy) ? (house2.energy - refHouse2Energy) : 0.0;

    // // 4. Combine house daily usage
    // totalHouseEnergy = dailyHouse1Usage + dailyHouse2Usage;

    // // 5. Calculate Daily Energy Loss / Theft
    // dailyEnergyLoss = dailyMasterUsage - totalHouseEnergy;
    // if (dailyEnergyLoss < 0) dailyEnergyLoss = 0.0;

    // // 6. Smarter Percentage-Based Daily Theft Check
    // lossPercentage = 0.0;
    // if (dailyMasterUsage > 0.05) {
    //     lossPercentage = (dailyEnergyLoss / dailyMasterUsage) * 100.0;
    //     isTheftDetected = (lossPercentage > THEFT_TOLERANCE_PERCENT);
    // } else {
    //     lossPercentage = 0.0;
    //     isTheftDetected = false; 
    // }


    // 4. Combine house daily usage
    totalHouseEnergy = dailyHouse1Usage + dailyHouse2Usage;

    // 5. Calculate Daily Energy Loss / Theft (ALLOW NEGATIVE FOR DEBUGGING)
    dailyEnergyLoss = dailyMasterUsage - totalHouseEnergy;
    // (Removed the line that forced it to 0.0 so you can see sensor drift in the Serial Monitor)

    // 6. Smarter Percentage-Based Daily Theft Check (WITH 10-SECOND DELAY)
    if (dailyMasterUsage > 0.05 && dailyEnergyLoss > 0) {
        // Normal state: Grid is active, and there is actual positive loss
        lossPercentage = (dailyEnergyLoss / dailyMasterUsage) * 100.0;
        
        if (lossPercentage > THEFT_TOLERANCE_PERCENT) {
            // Mismatch detected! Start the timer if it isn't running yet.
            if (theftConditionStartTime == 0) {
                theftConditionStartTime = millis();
            }
            
            // Only trigger the ALARM if the mismatch has lasted longer than 10 seconds
            if (millis() - theftConditionStartTime >= THEFT_CONFIRMATION_DELAY) {
                isTheftDetected = true;
            }
        } else {
            // Everything is balanced. Reset the timer and turn off alarm.
            theftConditionStartTime = 0;
            isTheftDetected = false;
        }
    } 
    else if (dailyEnergyLoss <= 0) {
        // Master is LESS than the houses. Reset timer and alarm.
        lossPercentage = 0.0;
        theftConditionStartTime = 0;
        isTheftDetected = false;
    }
    else {
        // Master hasn't reached the 0.05 kWh minimum threshold yet.
        lossPercentage = 0.0;
        theftConditionStartTime = 0;
        isTheftDetected = false; 
    }


    // Live terminal log showing Daily metrics and Loss percentage
    Serial.printf("Daily Grid: %.2fkWh | Daily H1: %.2fkWh | Daily H2: %.2fkWh | Loss: %.2fkWh (%.1f%%) | Status: %s\n", 
                  dailyMasterUsage, dailyHouse1Usage, dailyHouse2Usage, dailyEnergyLoss, lossPercentage,
                  isTheftDetected ? "ALARM (THEFT)" : "NORMAL");
}

// Helper function to read the 8-byte struct from a Slave
HouseData readSlaveI2C(uint8_t address) {
    HouseData data = {0.0f, 0.0f};
    
    uint8_t bytesReceived = SlaveBus.requestFrom(address, (uint8_t)sizeof(HouseData));
    
    if (bytesReceived == sizeof(HouseData)) {
        uint8_t buffer[sizeof(HouseData)];
        for (size_t i = 0; i < sizeof(HouseData); i++) {
            buffer[i] = SlaveBus.read();
        }
        memcpy(&data, buffer, sizeof(HouseData));
    } else {
        Serial.printf("Failed to read from House 0x%02X\n", address);
    }
    return data;
}

//======================================================================================
// UPDATE LCD (Original Format Preserved)
//======================================================================================
void updateLCD() {
    lcd.clear();

    // Row 0: House 1 Daily Energy
    lcd.setCursor(0, 0);
    lcd.print("H1: ");
    lcd.print(dailyHouse1Usage, 1);
    lcd.print("kWh");



    // Row 1: House 2 Daily Energy and Status
    lcd.setCursor(0, 1);
    lcd.print("H2: ");
    lcd.print(dailyHouse2Usage, 1);
    lcd.print("kWh");
    
    // Status positioned precisely at column 10
    lcd.setCursor(10, 1); 
    if (isTheftDetected) {
        lcd.print("ALARM!");
    } else {
        lcd.print("  NORM");
    }
}

//======================================================================================
// FIREBASE: UPLOAD DAILY ENERGY METRICS ON THEFT
//======================================================================================
void handleTheftAlert() {
    if (!app.ready()) return;

    if (isTheftDetected) {
        if (millis() - lastAlertTime >= 60000 || lastAlertTime == 0) {
            lastAlertTime = millis();

            Serial.println(">> THEFT DETECTED: Sending Daily Energy Loss to Firebase... <<");

            DynamicJsonDocument doc(256);
            doc["dailyMaster"]     = dailyMasterUsage;
            doc["dailyHouse1"]     = dailyHouse1Usage;
            doc["dailyHouse2"]     = dailyHouse2Usage;
            doc["dailyLossEnergy"] = dailyEnergyLoss;
            doc["lossPercentage"]  = lossPercentage;


        //    doc["timestamps"]       = millis() / 1000;

            // Tell Firebase to use its own server time
            JsonObject tsObj = doc.createNestedObject("timestamp");
            tsObj[".sv"] = "timestamp";

            String jsonString;
            serializeJson(doc, jsonString);
            object_t jsonPayload(jsonString.c_str());

            Database.push<object_t>(client, theftAlertsPath, jsonPayload);
            Database.set<String>(client, gridStatusPath, "THEFT_ACTIVE");
            
            Serial.println(">> Alert pushed successfully <<");
        }
    } else {
        if (lastAlertTime != 0) {
            Database.set<String>(client, gridStatusPath, "NORMAL");
            lastAlertTime = 0; 
        }
    }
}

//======================================================================================
// FIREBASE: UPLOAD MASTER LIVE METRICS (NEW)
//======================================================================================
void uploadMasterLiveMetrics() {
    if (!app.ready()) return;

    // Read all fresh metrics directly from the hardware
    float v  = pzemMaster.voltage();
    float c  = pzemMaster.current();
    float p  = pzemMaster.power();
    float e  = pzemMaster.energy();
    float f  = pzemMaster.frequency();
    float pf = pzemMaster.pf();
    

    DynamicJsonDocument doc(256);
    
    // Check for NaN (Not a Number) to prevent Firebase errors if sensor temporarily disconnects
    doc["current"]      = isnan(c) ? 0.0 : c;
    doc["energy"]       = isnan(e) ? 0.0 : e;
    doc["frequency"]    = isnan(f) ? 0.0 : f;
    doc["power"]        = isnan(p) ? 0.0 : p;
    doc["power_factor"] = isnan(pf) ? 0.0 : pf;
    doc["sendTimeMs"]   = millis();
    doc["voltage"]      = isnan(v) ? 0.0 : v;

    // --- NEW: Live Daily Metrics for the Dashboard ---
    doc["dailyMaster"]  = dailyMasterUsage;
    doc["dailyHouse1"]  = dailyHouse1Usage;
    doc["dailyHouse2"]  = dailyHouse2Usage;
    doc["Loss"] =  dailyEnergyLoss;
    doc["Loss Percentage"] =  lossPercentage;

    String jsonString;
    serializeJson(doc, jsonString);
    object_t jsonPayload(jsonString.c_str());

    // Continuously overwrite the 'live' folder so your dashboard is always real-time
    Database.set<object_t>(client, masterLivePath, jsonPayload);
}

// Check Firebase for remote reset command ("RESET" at master/command)
void checkFirebaseCommands() {
    if (!app.ready()) return;
    String cmd = Database.get<String>(client, commandPath);
    if (cmd == "RESET") {
        resetDailyBaselines();
        Database.set<String>(client, commandPath, "OK");
    }
}

//======================================================================================
// NETWORK HELPERS
//======================================================================================
void connectWiFi() {
    lcd.setCursor(0, 1);
    lcd.print("WiFi Connecting.");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500); 
    }
}

void setupFirebase() {
    sslClient.setInsecure();
#if defined(ESP32)
    sslClient.setConnectionTimeout(2000);
    sslClient.setHandshakeTimeout(10);
#endif
    initializeApp(client, app, getAuth(userAuth), authHandler, "authTask");
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
}

void authHandler(AsyncResult &result) {
    if (!result.available()) return;
}