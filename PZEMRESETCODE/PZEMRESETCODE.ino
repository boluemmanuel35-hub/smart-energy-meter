#include <PZEM004Tv30.h>
#include <Preferences.h>

// Standard PZEM pins for ESP32 Serial2
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17

HardwareSerial pzemSerial(2);
PZEM004Tv30 pzem(pzemSerial, PZEM_RX_PIN, PZEM_TX_PIN);

Preferences preferences;

void setup() {
    Serial.begin(115200);
    pzemSerial.begin(9600, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);

    Serial.println("\n--- SLAVE FACTORY RESET STARTING ---");
    delay(1000); 

    // 1. WIPE THE ESP32 NVS MEMORY
    Serial.println("Wiping Slave NVS Memory...");
    preferences.begin("energy_store", false);
    preferences.clear(); 
    preferences.end();
    Serial.println(">> SUCCESS: NVS Memory wiped. <<");

    // 2. WIPE THE PZEM HARDWARE MEMORY
    Serial.println("Wiping Slave PZEM Hardware Memory...");
    pzem.resetEnergy();
    
    delay(1000); 
    
    // 3. VERIFY
    float afterEnergy = pzem.energy();
    Serial.printf("Slave PZEM Energy after reset: %.3f kWh\n", afterEnergy);
    
    if (afterEnergy == 0.000) {
        Serial.println(">> SUCCESS: Slave is officially reset to ZERO! <<");
        Serial.println("\nYou can now re-upload your main Slave code.");
    } else {
        Serial.println(">> FAILED: PZEM did not reset. Check AC power to the PZEM. <<");
    }
}

void loop() {
    // Do nothing
}