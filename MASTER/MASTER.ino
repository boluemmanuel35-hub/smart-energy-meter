// /*
//  * =====================================================================================
//  * ESP32 MASTER: Dual I2C Controllers, Master PZEM, and Firebase Theft Detection
//  * =====================================================================================
//  */

// #define ENABLE_USER_AUTH
// #define ENABLE_DATABASE

// #include <Arduino.h>
// #include <WiFi.h>
// #include <WiFiClientSecure.h>
// #include <FirebaseClient.h>
// #include <ArduinoJson.h>
// #include <Wire.h>
// #include <LiquidCrystal_I2C.h>
// #include <PZEM004Tv30.h>

// //======================================================================================
// // CONFIGURATION
// //======================================================================================
// /* WiFi Credentials */
// #define WIFI_SSID     "RESEARCH LAB"
// #define WIFI_PASSWORD "G1234567890"

// /* Firebase Credentials */
// #define API_KEY       "AIzaSyBZLtf0wO0jivZXzfebOIehXAXuF40KCCA"
// #define DATABASE_URL  "https://esp32-918b2-default-rtdb.firebaseio.com/"
// #define USER_EMAIL    "esp32@gmail.com"
// #define USER_PASS     "emmy4012"

// /* Master PZEM Pins */
// #define PZEM_RX_PIN 16
// #define PZEM_TX_PIN 17

// /* Dual I2C Configuration */
// #define SLAVE_SDA 21
// #define SLAVE_SCL 22
// #define HOUSE1_ADDRESS 0x08
// #define HOUSE2_ADDRESS 0x09

// #define LCD_SDA 25
// #define LCD_SCL 26
// #define LCD_ADDRESS 0x27

// /* Theft Detection Tolerance (in PERCENTAGE - e.g., 5.0 = 5%) */
// const float THEFT_TOLERANCE_PERCENT = 5.0; 

// //======================================================================================
// // GLOBAL FIREBASE PATHS
// //======================================================================================
// String theftAlertsPath = "master/theft_alerts";
// String gridStatusPath  = "master/status";

// //======================================================================================
// // GLOBAL OBJECTS
// //======================================================================================
// /* I2C Buses */
// TwoWire SlaveBus = TwoWire(1); // Controller 1 for Slaves
// LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2); // LCD on Controller 0 (Wire)

// /* Master PZEM */
// HardwareSerial pzemSerial(2);
// PZEM004Tv30 pzemMaster(pzemSerial, PZEM_RX_PIN, PZEM_TX_PIN);

// /* Firebase */
// UserAuth userAuth(API_KEY, USER_EMAIL, USER_PASS);
// WiFiClientSecure sslClient;
// FirebaseApp app;
// RealtimeDatabase Database;
// using AsyncClient = AsyncClientClass;
// AsyncClient client(sslClient);

// //======================================================================================
// // VARIABLES
// //======================================================================================
// float masterEnergy = 0.0;
// float masterCurrent = 0.0; 
// float house1Energy = 0.0;
// float house2Energy = 0.0;
// float totalHouseEnergy = 0.0;
// float unaccountedEnergy = 0.0;
// bool isTheftDetected = false;

// unsigned long lastReadTime = 0;
// unsigned long lastAlertTime = 0;

// //======================================================================================
// // FUNCTION PROTOTYPES
// //======================================================================================
// void connectWiFi();
// void setupFirebase();
// void authHandler(AsyncResult &result);
// void fetchEnergies();
// float readSlaveI2C(uint8_t address);
// void updateLCD();
// void handleTheftAlert();

// //======================================================================================
// // SETUP
// //======================================================================================
// void setup() {
//     Serial.begin(115200);

//     // 1. START LCD I2C (Controller 0)
//     Wire.begin(LCD_SDA, LCD_SCL, 100000);
//     lcd.init();
//     lcd.backlight();
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("MASTER BOOTING");

//     // 2. START SLAVE I2C (Controller 1)
//     SlaveBus.begin(SLAVE_SDA, SLAVE_SCL, 100000);

//     // 3. Initialize Master PZEM
//     pzemSerial.begin(9600, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);

//     // 4. Network & Cloud Setup
//     connectWiFi();
//     setupFirebase();

//     lcd.clear();
//     lcd.print("SYSTEM READY");
//     delay(2000);
// }

// //======================================================================================
// // LOOP
// //======================================================================================
// void loop() {
//     app.loop();
//     unsigned long now = millis();

//     // Run the auditing cycle every 3 seconds
//     if (now - lastReadTime >= 3000) {
//         lastReadTime = now;
        
//         fetchEnergies();
//         updateLCD();
//         handleTheftAlert();
//     }
// }

// //======================================================================================
// // FETCH DATA (Master PZEM + Both Slaves)
// //======================================================================================
// void fetchEnergies() {
//     // 1. Read Master Energy & Current (Main Grid)
//     float mE = pzemMaster.energy();
//     if (!isnan(mE)) {
//         masterEnergy = mE;
//     }
    
//     float mI = pzemMaster.current();
//     if (!isnan(mI)) {
//         masterCurrent = mI;
//     }

//     // 2. Request Data from House 1 and House 2
//     house1Energy = readSlaveI2C(HOUSE1_ADDRESS);
//     delay(50); 
//     house2Energy = readSlaveI2C(HOUSE2_ADDRESS);

//     // 3. Combine house usage
//     totalHouseEnergy = house1Energy + house2Energy;

//     // 4. Calculate Theft / Loss
//     unaccountedEnergy = masterEnergy - totalHouseEnergy;

//     // Prevent negative display if sensors slightly drift naturally
//     if (unaccountedEnergy < 0) unaccountedEnergy = 0.0;

//     // 5. SMARTER PERCENTAGE-BASED THEFT CHECK
//     float lossPercentage = 0.0;
    
//     // Only calculate percentage if Master has recorded a solid baseline (> 0.05 kWh)
//     // This stops the "micro-float" division that causes 100% false alarms when empty.
//     if (masterEnergy > 0.05) {
//         lossPercentage = (unaccountedEnergy / masterEnergy) * 100.0;
//         isTheftDetected = (lossPercentage > THEFT_TOLERANCE_PERCENT);
//     } else {
//         lossPercentage = 0.0; // Force it to 0% when the grid is basically empty
//         isTheftDetected = false; 
//     }

//     // Print Grid Energy, Current, and the Loss Percentage live!
//     Serial.printf("Grid: %.2fkWh (%.2fA) | H1: %.2fkWh | H2: %.2fkWh | Loss: %.2fkWh (%.1f%%)\n", 
//                    masterEnergy, masterCurrent, house1Energy, house2Energy, unaccountedEnergy, lossPercentage);
// }

// // Helper function to read from a specific Slave address
// float readSlaveI2C(uint8_t address) {
//     float readValue = 0.0; 
    
//     // Request exactly 4 bytes (size of float)
//     uint8_t bytesReceived = SlaveBus.requestFrom(address, (uint8_t)sizeof(float));
    
//     if (bytesReceived == sizeof(float)) {
//         uint8_t buffer[sizeof(float)];
//         for (int i = 0; i < sizeof(float); i++) {
//             buffer[i] = SlaveBus.read();
//         }
//         // Reconstruct the math float correctly
//         memcpy(&readValue, buffer, sizeof(float));
//     } else {
//         Serial.printf("Failed to read from House 0x%02X\n", address);
//     }
//     return readValue;
// }

// //======================================================================================
// // UPDATE LCD
// //======================================================================================
// void updateLCD() {
//     lcd.clear();

//     // Row 0: House 1 Energy
//     lcd.setCursor(0, 0);
//     lcd.print("H1: ");
//     lcd.print(house1Energy, 1);
//     lcd.print("kWh");

//     // Row 1: House 2 Energy and Status
//     lcd.setCursor(0, 1);
//     lcd.print("H2: ");
//     lcd.print(house2Energy, 1);
//     lcd.print("kWh");
    
//     // Status positioned precisely at column 10 to fit perfectly
//     lcd.setCursor(10, 1); 
//     if (isTheftDetected) {
//         lcd.print("ALARM!");
//     } else {
//         lcd.print("  NORM");
//     }
// }

// //======================================================================================
// // FIREBASE: ONLY UPLOAD ON THEFT
// //======================================================================================
// void handleTheftAlert() {
//     if (!app.ready()) return;

//     if (isTheftDetected) {
//         if (millis() - lastAlertTime >= 60000 || lastAlertTime == 0) {
//             lastAlertTime = millis();

//             Serial.println(">> THEFT DETECTED: Sending Alert to Firebase... <<");

//             DynamicJsonDocument doc(256);
//             doc["masterTotal"] = masterEnergy;
//             doc["house1"]      = house1Energy;
//             doc["house2"]      = house2Energy;
//             doc["stolenEnergy"]= unaccountedEnergy;
//             doc["timestamp"]   = millis() / 1000;

//             String jsonString;
//             serializeJson(doc, jsonString);
//             object_t jsonPayload(jsonString.c_str());

//             Database.push<object_t>(client, theftAlertsPath, jsonPayload);
//             Database.set<String>(client, gridStatusPath, "THEFT_ACTIVE");
            
//             Serial.println(">> Alert pushed successfully <<");
//         }
//     } else {
//         if (lastAlertTime != 0) {
//             Database.set<String>(client, gridStatusPath, "NORMAL");
//             lastAlertTime = 0; 
//         }
//     }
// }

// //======================================================================================
// // NETWORK HELPERS
// //======================================================================================
// void connectWiFi() {
//     lcd.setCursor(0, 1);
//     lcd.print("WiFi Connecting.");
//     WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//     while (WiFi.status() != WL_CONNECTED) {
//         delay(500); 
//     }
// }

// void setupFirebase() {
//     sslClient.setInsecure();
// #if defined(ESP32)
//     sslClient.setConnectionTimeout(2000);
//     sslClient.setHandshakeTimeout(10);
// #endif
//     initializeApp(client, app, getAuth(userAuth), authHandler, "authTask");
//     app.getApp<RealtimeDatabase>(Database);
//     Database.url(DATABASE_URL);
// }

// void authHandler(AsyncResult &result) {
//     if (!result.available()) return;
// }












/*
 * =====================================================================================
 * ESP32 MASTER: Dual I2C Controllers, Master PZEM, Current Audit & Energy LCD Display
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

/* Current Theft Threshold (Amperes) */
const float THEFT_CURRENT_THRESHOLD = 0.035; 

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

//======================================================================================
// GLOBAL OBJECTS
//======================================================================================
/* I2C Buses */
TwoWire SlaveBus = TwoWire(1); // Controller 1 for Slaves
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2); // LCD on Controller 0 (Wire)

/* Master PZEM */
HardwareSerial pzemSerial(2);
PZEM004Tv30 pzemMaster(pzemSerial, PZEM_RX_PIN, PZEM_TX_PIN);

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

float totalHouseCurrent = 0.0;
float unaccountedCurrent = 0.0;
bool isTheftDetected = false;

unsigned long lastReadTime = 0;
unsigned long lastAlertTime = 0;

//======================================================================================
// FUNCTION PROTOTYPES
//======================================================================================
void connectWiFi();
void setupFirebase();
void authHandler(AsyncResult &result);
void fetchEnergiesAndCurrents();
HouseData readSlaveI2C(uint8_t address);
void updateLCD();
void handleTheftAlert();

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

    // 4. Network & Cloud Setup
    connectWiFi();
    setupFirebase();

    lcd.clear();
    lcd.print("SYSTEM READY");
    delay(2000);
}

//======================================================================================
// LOOP
//======================================================================================
void loop() {
    app.loop();
    unsigned long now = millis();

    // Run the auditing cycle every 3 seconds
    if (now - lastReadTime >= 3000) {
        lastReadTime = now;
        
        fetchEnergiesAndCurrents();
        updateLCD();
        handleTheftAlert();
    }
}

//======================================================================================
// FETCH DATA (Master PZEM + Both Slaves)
//======================================================================================
void fetchEnergiesAndCurrents() {
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

    // 3. Combine live house currents
    totalHouseCurrent = house1.current + house2.current;

// 4. Calculate Absolute Difference (Magnitude of mismatch)
    unaccountedCurrent = fabs(masterCurrent - totalHouseCurrent);

    // Zero out negative drift
    if (unaccountedCurrent < 0.0) {
        unaccountedCurrent = 0.0;
    }

    // 5. Theft Check against absolute threshold (0.20 A)
    isTheftDetected = (unaccountedCurrent >= THEFT_CURRENT_THRESHOLD);

    // Live terminal log
    Serial.printf("Grid: %.2fkWh (%.2fA) | H1: %.2fkWh (%.2fA) | H2: %.2fkWh (%.2fA) | Unmetered: %.2fA | Status: %s\n", 
                  masterEnergy, masterCurrent, 
                  house1.energy, house1.current, 
                  house2.energy, house2.current, 
                  unaccountedCurrent, 
                  isTheftDetected ? "ALARM (THEFT)" : "NORMAL");
}

// Helper function to read the 8-byte struct from a Slave
HouseData readSlaveI2C(uint8_t address) {
    HouseData data = {0.0f, 0.0f};
    
    // Request exactly 8 bytes (sizeof struct HouseData)
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
// UPDATE LCD (Original Format)
//======================================================================================
void updateLCD() {
    lcd.clear();

    // Row 0: House 1 Energy
    lcd.setCursor(0, 0);
    lcd.print("H1: ");
    lcd.print(house1.energy, 1);
    lcd.print("kWh");

    // Row 1: House 2 Energy and Status
    lcd.setCursor(0, 1);
    lcd.print("H2: ");
    lcd.print(house2.energy, 1);
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
// FIREBASE: ONLY UPLOAD ON THEFT
//======================================================================================
void handleTheftAlert() {
    if (!app.ready()) return;

    if (isTheftDetected) {
        if (millis() - lastAlertTime >= 60000 || lastAlertTime == 0) {
            lastAlertTime = millis();

            Serial.println(">> THEFT DETECTED: Sending Alert to Firebase... <<");

            DynamicJsonDocument doc(256);
            doc["masterEnergy"]      = masterEnergy;
            doc["masterCurrent"]     = masterCurrent;
            doc["house1Energy"]      = house1.energy;
            doc["house1Current"]     = house1.current;
            doc["house2Energy"]      = house2.energy;
            doc["house2Current"]     = house2.current;
            doc["unmeteredCurrent"]  = unaccountedCurrent;
            doc["timestamp"]         = millis() / 1000;

            String jsonString;
            serializeJson(doc, jsonString);
            object_t jsonPayload(jsonString.c_str());

            Database.set<object_t>(client, theftAlertsPath, jsonPayload);
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