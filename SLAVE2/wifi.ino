void connectWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500); 
    }
    Serial.println("\nIP: " + WiFi.localIP().toString());
}

void syncTime() {
    Serial.print("Synchronizing NTP time...");
    configTime(3600, 0, "pool.ntp.org", "time.nist.gov"); // UTC+1
    struct tm timeinfo;
    int retry = 0;
    while (!getLocalTime(&timeinfo) && retry < 10) {
        Serial.print(".");
        delay(1000);
        retry++;
    }
    if (retry < 10) Serial.println(" TIME synchronized!");
    else Serial.println(" NTP sync failed!");
}

String getISOTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "";
    char buf[25];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    return String(buf);
}

unsigned long long getUnixTimestampMs() {
    time_t now = time(nullptr);
    return (unsigned long long)now * 1000ULL;
}