/*
 * ==========================================================
 * BLOCKCHAIN HELPERS & PUSH FUNCTION
 * ==========================================================
 */

void sendEnergyToBackend(float energy) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(">> WiFi not connected. Cannot send to Blockchain.");
        return;
    }

    // getISOTime() is called from your wifi.ino tab
    String isoDate = getISOTime();
    if (isoDate.isEmpty()) {
        Serial.println(">> Error: Could not get current timestamp.");
        return;
    }

    Serial.println("\n==========================================");
    Serial.println("SENDING DAILY ENERGY TO SOLANA BACKEND");
    Serial.println("==========================================");

    HTTPClient http;
    http.begin(BACKEND_URL);
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument doc(256);
    doc["meterId"] = METER_ID;
    doc["energy"]  = energy;
    doc["date"]    = isoDate;

    String body;
    serializeJson(doc, body);

    Serial.printf("POST URL : %s\n", BACKEND_URL);
    Serial.printf("Payload  : %s\n", body.c_str());

    int responseCode = http.POST(body);

    Serial.printf("HTTP Response Code: %d\n", responseCode);
    if (responseCode > 0) {
        String response = http.getString();
        Serial.println("Backend Reply: " + response);
    } else {
        Serial.printf(">> Error failed to connect. Error: %s\n", http.errorToString(responseCode).c_str());
    }

    http.end();
    Serial.println("==========================================");
}