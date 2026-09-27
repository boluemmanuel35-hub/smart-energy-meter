/*
 * ==========================================================
 * DAILY ENERGY PREDICTION (LINEAR REGRESSION)
 * ==========================================================
 */
void calculateAndUploadPrediction(struct tm &timeinfo)
{
    char dateBuffer[11];
    strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &timeinfo);
    String currentDateStr = String(dateBuffer);

    preferences.begin("energy_store", false);
    int currentDayIndex = preferences.getInt("regDay", 32); // Reset default to start from 32
    
    // Linear Regression Formula
    float predictedEnergy = 25.5494 + (-0.0758 * (float)currentDayIndex);
    if (predictedEnergy < 0) predictedEnergy = 0.0;

    unsigned long long unixEpochMs = getUnixTimestampMs();

    char timeBuffer[20];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    String timestampStr = String(timeBuffer);

    DynamicJsonDocument doc(256);
    doc["dayIndex"]        = currentDayIndex;
    doc["predictedEnergy"] = predictedEnergy;
    doc["timestamp"]       = timestampStr;
    doc["sendTimeMs"]      = unixEpochMs;

    String jsonString;
    serializeJson(doc, jsonString);
    object_t jsonPayload(jsonString.c_str());

    Database.push<object_t>(client, predictionPath, jsonPayload);

    lastPredictedDate = currentDateStr;
    predictionCompletedToday = true;

    currentDayIndex++;
    preferences.putInt("regDay", currentDayIndex);
    preferences.putString("lastPredDate", currentDateStr);
    preferences.putBool("predCompleted", true);
    preferences.putBool("pendPredFlag", false);
    preferences.end();

    Serial.printf(">> Success: Daily Prediction for Day %d (%.2fkWh) uploaded! <<\n", currentDayIndex - 1, predictedEnergy);
}


/*
 * ==========================================================
 * UPLOAD PENDING PREDICTION DATA WHEN RECONNECTED
 * ==========================================================
 */
void uploadPendingPrediction()
{
    if (!app.ready()) return;

    Serial.println("Uploading pending daily prediction record to Firebase...");

    struct tm timeinfo;
    String timestampStr = "";
    if (getLocalTime(&timeinfo)) {
        char timeBuffer[20];
        strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        timestampStr = String(timeBuffer);
    } else {
        timestampStr = "22:00:00";
    }
    
    unsigned long long unixEpochMs = getUnixTimestampMs();

    DynamicJsonDocument doc(256);
    doc["dayIndex"]        = pendingPredDayIndex;
    doc["predictedEnergy"] = pendingPredEnergy;
    doc["timestamp"]       = timestampStr;
    doc["sendTimeMs"]      = unixEpochMs;

    String jsonString;
    serializeJson(doc, jsonString);
    object_t jsonPayload(jsonString.c_str());

    Database.push<object_t>(client, predictionPath, jsonPayload);

    preferences.begin("energy_store", false);
    int nextDayIndex = pendingPredDayIndex + 1;
    preferences.putInt("regDay", nextDayIndex);
    preferences.putBool("pendPredFlag", false);
    preferences.putBool("predCompleted", true);
    preferences.end();

    pendingPredictionFlag = false;
    predictionCompletedToday = true;

    Serial.println(">> Success: Pending prediction successfully recovered and uploaded! <<");
}