// void setupI2CSlave() {
//     Wire.begin(I2C_SLAVE_ADDR, SDA_PIN, SCL_PIN, 100000);
//     Wire.onRequest(requestEvent);
//     Serial.println("I2C Slave started on address 0x08");
// }

// void requestEvent() {
//     // Instantly serve the freshest accumulated energy value
//     // This takes microseconds, keeping the I2C bus happy and preventing crashes
//     uint8_t buffer[sizeof(float)];
//     memcpy(buffer, &currentAccumulatedEnergy, sizeof(float));
    
//     Wire.write(buffer, sizeof(float));
// }











/*
 * ==========================================================
 * SENSOR POLLING & I2C HANDLERS
 * ==========================================================
 */
void updateSensorReadings() {
    float e = pzem.energy();
    float i = pzem.current();

    if (!isnan(e)) {
        latestHouseData.energy = e;
        currentAccumulatedEnergy = e; // Keeps daily_NVS and firebase tabs synchronized
    }
    if (!isnan(i)) {
        latestHouseData.current = i;
    }
}

void setupI2CSlave() {
    Wire.begin(I2C_SLAVE_ADDR, SDA_PIN, SCL_PIN, 100000);
    Wire.onRequest(requestEvent);
    Serial.printf("I2C Slave started on address 0x%02X\n", I2C_SLAVE_ADDR);
}

void requestEvent() {
    // Instantly write the 8-byte HouseData struct
    Wire.write((uint8_t*)&latestHouseData, sizeof(HouseData));
}