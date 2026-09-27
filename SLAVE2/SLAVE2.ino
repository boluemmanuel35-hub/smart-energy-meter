// /*
//  * ==========================================================
//  * ESP32 SLAVE: PZEM, Firebase, Blockchain, NVS, I2C & Relay
//  * ==========================================================
//  */

// #define ENABLE_USER_AUTH
// #define ENABLE_DATABASE

// #include <Arduino.h>
// #include <WiFi.h>
// #include <WiFiClientSecure.h>
// #include <FirebaseClient.h>
// #include <ArduinoJson.h>
// #include <PZEM004Tv30.h>
// #include <Preferences.h>
// #include <time.h>
// #include <Wire.h>
// #include <HTTPClient.h>

// /* Credentials & Config */
// #define WIFI_SSID     "RESEARCH LAB"
// #define WIFI_PASSWORD "G1234567890"
// #define API_KEY       "AIzaSyBZLtf0wO0jivZXzfebOIehXAXuF40KCCA"
// #define DATABASE_URL  "https://esp32-918b2-default-rtdb.firebaseio.com/"
// #define USER_EMAIL    "esp32@gmail.com"
// #define USER_PASS     "emmy4012"
// #define BACKEND_URL   "https://energy-blockchain.onrender.com/api/blockchain/submit"

// /* Hardware Pins */
// #define LED_PIN 2
// #define RELAY_PIN 4
// #define PZEM_RX_PIN 16
// #define PZEM_TX_PIN 17

// /* I2C Settings */
// #define I2C_SLAVE_ADDR 0x09
// #define SDA_PIN 21
// #define SCL_PIN 22

// /* Database Paths */
// String infoPath   = "user2/info/";
// String meterPath  = "user2/meter/";
// String energyPath = "user2/dailyEnergy/";
// String relayPath  = "user2/relay/state"; 
// String predictionPath = "user2/predictions/";

// /* Variables for Daily Prediction Tracking */
// String lastPredictedDate = "";
// bool predictionCompletedToday = false;
// bool pendingPredictionFlag = false;
// int pendingPredDayIndex = 32;
// float pendingPredEnergy = 0.0;

// /* Global Objects */
// HardwareSerial pzemSerial(2);
// PZEM004Tv30 pzem(pzemSerial, PZEM_RX_PIN, PZEM_TX_PIN);

// Preferences preferences;
// UserAuth userAuth(API_KEY, USER_EMAIL, USER_PASS);
// WiFiClientSecure sslClient;
// FirebaseApp app;
// RealtimeDatabase Database;
// using AsyncClient = AsyncClientClass;
// AsyncClient client(sslClient);

// /* Global Variables */
// float currentAccumulatedEnergy = 0.0;
// unsigned long previousUploadMillis = 0;
// const unsigned long uploadInterval = 10000;
// unsigned long previousHistoryMillis = 0;
// const unsigned long historyInterval = 300000; 


// /* Daily Trackers */
// float savedPreviousEnergy = 0.0;
// String lastUploadedDate = "";
// bool uploadCompletedToday = false;
// bool pendingUploadFlag = false;
// float pendingDailyEnergy = 0.0;
// float pendingCumulativeEnergy = 0.0;
// String pendingDateStr = "";
// unsigned long lastTimeCheckMillis = 0;
// const unsigned long timeCheckInterval = 10000;


// /*
//  * ==========================================================
//  * FUNCTION PROTOTYPES (For Tabbed Project Structure)
//  * ==========================================================
//  */
// void connectWiFi();
// void syncTime();
// void loadSavedData();
// void setupFirebase();
// void setupI2CSlave();
// void uploadPendingData();
// void uploadPendingPrediction();
// void checkDailyUpload();
// void checkRelayState();
// void writeBoardInfo();
// void readAndUploadMeterData(unsigned long currentTime);


// /*
//  * ==========================================================
//  * SETUP
//  * ==========================================================
//  */
// void setup() {
//     Serial.begin(115200);
//     pinMode(LED_PIN, OUTPUT);
    
//     // Relay Setup
//     pinMode(RELAY_PIN, OUTPUT);
//     digitalWrite(RELAY_PIN, LOW);
    
//     pzemSerial.begin(9600, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);

//     connectWiFi();
//     syncTime();
//     loadSavedData();
//     setupFirebase();
//     setupI2CSlave();
    
//     // Check immediately on boot if any pending uploads exist and network is ready
//     if (pendingUploadFlag && WiFi.status() == WL_CONNECTED) {
//         uploadPendingData();
//     }
    
//     if (pendingPredictionFlag && WiFi.status() == WL_CONNECTED) {
//         uploadPendingPrediction();
//     }
// }

// /*
//  * ==========================================================
//  * LOOP
//  * ==========================================================
//  */
// void loop() {
//     app.loop();
//     unsigned long now = millis();
    
//     // Non-blocking LED blink
//     static unsigned long previousBlink = 0;
//     static bool ledState = false;
//     if (now - previousBlink >= 1000) {
//         previousBlink = now;
//         ledState = !ledState;
//         digitalWrite(LED_PIN, ledState);
//     }

//     // Frequently fetch the native accumulating energy from the PZEM for I2C
//     static unsigned long lastPzemRead = 0;
//     if (now - lastPzemRead >= 2000) { // Fetch every 2 seconds
//         lastPzemRead = now;
//         float e = pzem.energy();
//         if (!isnan(e)) {
//             currentAccumulatedEnergy = e; // Keep the serving variable fresh
//         }
//     }

//     // Continuous check for background network recovery and pending uploads
//     if (WiFi.status() == WL_CONNECTED) {
//         if (pendingUploadFlag) {
//             uploadPendingData();
//         }
//         if (pendingPredictionFlag) {
//             uploadPendingPrediction();
//         }
//     }

//     if (now - lastTimeCheckMillis >= timeCheckInterval || lastTimeCheckMillis == 0) {
//         lastTimeCheckMillis = now;
//         checkDailyUpload();
//         checkRelayState();
//     }

//     // Stop here if Firebase isn't ready (saves uploading without connection)
//     if (!app.ready()) return;

//     if (now - previousUploadMillis >= uploadInterval) {
//         previousUploadMillis = now;
//         writeBoardInfo();
//         readAndUploadMeterData(now);
//     }
// }






/*
 * ==========================================================
 * ESP32 SLAVE: PZEM, Firebase, Blockchain, NVS, I2C & Relay
 * ==========================================================
 */

#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <ArduinoJson.h>
#include <PZEM004Tv30.h>
#include <Preferences.h>
#include <time.h>
#include <Wire.h>
#include <HTTPClient.h>

/* Credentials & Config */
#define WIFI_SSID     "RESEARCH LAB"
#define WIFI_PASSWORD "G1234567890"
#define API_KEY       "AIzaSyBZLtf0wO0jivZXzfebOIehXAXuF40KCCA"
#define DATABASE_URL  "https://esp32-918b2-default-rtdb.firebaseio.com/"
#define USER_EMAIL    "esp32@gmail.com"
#define USER_PASS     "emmy4012"
#define BACKEND_URL   "https://energy-blockchain.onrender.com/api/blockchain/submit"

/* Hardware Pins */
#define LED_PIN 2
#define RELAY_PIN 4
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17

/* I2C Settings */
#define I2C_SLAVE_ADDR 0x09   // Change to 0x08 when uploading to House 1
#define SDA_PIN 21
#define SCL_PIN 22

/* 8-Byte Data Structure matching Master */
struct HouseData {
    float energy;   // kWh
    float current;  // Amperes
};
volatile HouseData latestHouseData = {0.0f, 0.0f};

/* Database Paths */
String infoPath       = "user2/info/";
String meterPath      = "user2/meter/";
String energyPath     = "user2/dailyEnergy/";
String relayPath      = "user2/relay/state"; 
String predictionPath = "user2/predictions/";

/* Variables for Daily Prediction Tracking */
String lastPredictedDate = "";
bool predictionCompletedToday = false;
bool pendingPredictionFlag = false;
int pendingPredDayIndex = 32;
float pendingPredEnergy = 0.0;

/* Global Objects */
HardwareSerial pzemSerial(2);
PZEM004Tv30 pzem(pzemSerial, PZEM_RX_PIN, PZEM_TX_PIN);

Preferences preferences;
UserAuth userAuth(API_KEY, USER_EMAIL, USER_PASS);
WiFiClientSecure sslClient;
FirebaseApp app;
RealtimeDatabase Database;
using AsyncClient = AsyncClientClass;
AsyncClient client(sslClient);

/* Global Variables (Required by other tabs: daily_NVS & firebase) */
float currentAccumulatedEnergy = 0.0;
unsigned long previousUploadMillis = 0;
const unsigned long uploadInterval = 10000;
unsigned long previousHistoryMillis = 0;
const unsigned long historyInterval = 300000; 

/* Daily Trackers */
float savedPreviousEnergy = 0.0;
String lastUploadedDate = "";
bool uploadCompletedToday = false;
bool pendingUploadFlag = false;
float pendingDailyEnergy = 0.0;
float pendingCumulativeEnergy = 0.0;
String pendingDateStr = "";
unsigned long lastTimeCheckMillis = 0;
const unsigned long timeCheckInterval = 10000;

/*
 * ==========================================================
 * FUNCTION PROTOTYPES (For Tabbed Project Structure)
 * ==========================================================
 */
void connectWiFi();
void syncTime();
void loadSavedData();
void setupFirebase();
void setupI2CSlave();
void requestEvent();
void updateSensorReadings();
void uploadPendingData();
void uploadPendingPrediction();
void checkDailyUpload();
void checkRelayState();
void writeBoardInfo();
void readAndUploadMeterData(unsigned long currentTime);

/*
 * ==========================================================
 * SETUP
 * ==========================================================
 */
void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    
    // Relay Setup
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    
    pzemSerial.begin(9600, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);

    connectWiFi();
    syncTime();
    loadSavedData();
    setupFirebase();
    setupI2CSlave();
    
    // Check immediately on boot if any pending uploads exist and network is ready
    if (pendingUploadFlag && WiFi.status() == WL_CONNECTED) {
        uploadPendingData();
    }
    
    if (pendingPredictionFlag && WiFi.status() == WL_CONNECTED) {
        uploadPendingPrediction();
    }
}

/*
 * ==========================================================
 * LOOP
 * ==========================================================
 */
void loop() {
    app.loop();
    unsigned long now = millis();
    
    // Non-blocking LED blink
    static unsigned long previousBlink = 0;
    static bool ledState = false;
    if (now - previousBlink >= 1000) {
        previousBlink = now;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
    }

    // Refresh Energy and Current every 1.5 seconds for I2C Master requests
    static unsigned long lastPzemRead = 0;
    if (now - lastPzemRead >= 1500) {
        lastPzemRead = now;
        updateSensorReadings();
    }

    // Continuous check for background network recovery and pending uploads
    if (WiFi.status() == WL_CONNECTED) {
        if (pendingUploadFlag) {
            uploadPendingData();
        }
        if (pendingPredictionFlag) {
            uploadPendingPrediction();
        }
    }

    if (now - lastTimeCheckMillis >= timeCheckInterval || lastTimeCheckMillis == 0) {
        lastTimeCheckMillis = now;
        checkDailyUpload();
        checkRelayState();
    }

    // Stop here if Firebase isn't ready (saves uploading without connection)
    if (!app.ready()) return;

    if (now - previousUploadMillis >= uploadInterval) {
        previousUploadMillis = now;
        writeBoardInfo();
        readAndUploadMeterData(now);
    }
}
