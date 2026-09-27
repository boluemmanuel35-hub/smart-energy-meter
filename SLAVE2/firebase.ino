void authHandler(AsyncResult &result) {
    if (!result.available()) return;
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

void writeBoardInfo() {
    Database.set<String>(client, infoPath + "status", "ESP32 Online");
    Database.set<int>(client, infoPath + "uptime", millis() / 1000);
}

void readAndUploadMeterData(unsigned long currentTime) {
    float voltage = pzem.voltage();
    float current = pzem.current();
    float power   = pzem.power();
    float energy  = pzem.energy();
    float freq    = pzem.frequency();
    float pf      = pzem.pf();

    if (isnan(voltage)) {
        Serial.println(">> ERROR: Failed to read from PZEM sensor! <<");
        Database.set<String>(client, meterPath + "error", "Sensor Read Error");
        return;
    }

    currentAccumulatedEnergy = energy; // Update global for I2C and Daily calculations

    unsigned long long unixEpochMs = getUnixTimestampMs();
    DynamicJsonDocument doc(256);
    doc["sendTimeMs"]   = unixEpochMs;
    doc["voltage"]      = voltage;
    doc["current"]      = current;
    doc["power"]        = power;
    doc["energy"]       = energy;
    doc["frequency"]    = freq;
    doc["power_factor"] = pf;

    String jsonString;
    serializeJson(doc, jsonString);
    object_t jsonPayload(jsonString.c_str());

    Database.set<object_t>(client, meterPath + "metrics", jsonPayload);
    Database.set<String>(client, meterPath + "error", "None");
// Print reading locally
    Serial.printf("V: %.1fV | I: %.2fA | P: %.1fW | E: %.2fkWh\n", voltage, current, power, energy);
    Serial.println(">> SUCCESS: Real-time metrics pushed to Firebase! <<");
    if (currentTime - previousHistoryMillis >= historyInterval || previousHistoryMillis == 0) {
        previousHistoryMillis = currentTime;
        Database.push<object_t>(client, meterPath + "history", jsonPayload);
    }
}