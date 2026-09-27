void sendEnergyToBackend(float energy) {
 if (WiFi.status() != WL_CONNECTED) {
        Serial.println(">> ERROR: WiFi down. Skipping Blockchain push. <<");
        return;
    }
    String isoDate = getISOTime();
    if (isoDate == "") return;
Serial.println("Preparing to send daily energy to Solana Blockchain...");
    HTTPClient http;
    http.begin(BACKEND_URL);
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument doc(256);
    doc["meterId"] = "ESP32-001";
    doc["energy"]  = energy;
    doc["date"]    = isoDate; 

    String body;
    serializeJson(doc, body);

    int responseCode = http.POST(body);
    // Serial.print("Blockchain Backend Response Code: ");
    // Serial.println(responseCode);
    if (responseCode > 0) {
        Serial.printf(">> SUCCESS: Blockchain Backend Response Code: %d <<\n", responseCode);
        Serial.println("Backend Reply: " + http.getString());
    } else {
        Serial.printf(">> ERROR: Blockchain push failed. Code: %d <<\n", responseCode);
    }
    http.end();
}