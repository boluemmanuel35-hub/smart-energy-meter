/*
 * ==========================================================
 * NVS STORAGE & DAILY SCHEDULE (ENERGY + PREDICTION)
 * ==========================================================
 */

void loadSavedData() {
    preferences.begin("energy_store", false);
    
    // 1. Load Energy Records
    savedPreviousEnergy = preferences.getFloat("prevEnergy", 0.0);
    lastUploadedDate = preferences.getString("lastDate", "");
    uploadCompletedToday = preferences.getBool("completed", false);
    pendingUploadFlag = preferences.getBool("pendFlag", false);
    pendingDailyEnergy = preferences.getFloat("pendDaily", 0.0);
    pendingCumulativeEnergy = preferences.getFloat("pendCumul", 0.0);
    pendingDateStr = preferences.getString("pendDate", "");

    // 2. Load Prediction Records
    lastPredictedDate = preferences.getString("lastPredDate", "");
    predictionCompletedToday = preferences.getBool("predCompleted", false);
    pendingPredictionFlag = preferences.getBool("pendPredFlag", false);
    pendingPredDayIndex = preferences.getInt("pendPredDay", 32); // Default starts at day 32
    pendingPredEnergy = preferences.getFloat("pendPredEn", 0.0);
    
    preferences.end();
    
    Serial.println("Loaded saved persistent energy and prediction records from NVS.");
}

/* --- ENERGY NVS SAVING --- */

void saveCurrentEnergy(float cumulativeEnergy, String dateStr) {
    savedPreviousEnergy = cumulativeEnergy;
    lastUploadedDate = dateStr;
    
    preferences.begin("energy_store", false);
    preferences.putFloat("prevEnergy", cumulativeEnergy);
    preferences.putString("lastDate", dateStr);
    preferences.putBool("completed", true);
    preferences.end();
}

void savePendingUpload(float dailyEnergy, float cumulativeEnergy, String dateStr) {
    pendingUploadFlag = true;
    pendingDailyEnergy = dailyEnergy;
    pendingCumulativeEnergy = cumulativeEnergy;
    pendingDateStr = dateStr;
    
    preferences.begin("energy_store", false);
    preferences.putBool("pendFlag", true);
    preferences.putFloat("pendDaily", dailyEnergy);
    preferences.putFloat("pendCumul", cumulativeEnergy);
    preferences.putString("pendDate", dateStr);
    preferences.end();
}

void uploadPendingData() {
    if (!app.ready()) return;
    String timestampStr = pendingDateStr + " 22:00:00";
    unsigned long long unixEpochMs = getUnixTimestampMs();
    
    DynamicJsonDocument doc(256);
    doc["energy"] = pendingDailyEnergy;
    doc["cumulativeEnergy"] = pendingCumulativeEnergy;
    doc["timestamp"] = timestampStr;
    doc["sendTimeMs"] = unixEpochMs;

    String jsonString;
    serializeJson(doc, jsonString);
    object_t jsonPayload(jsonString.c_str());

    Database.push<object_t>(client, energyPath, jsonPayload); // Push to create unique ID
    
    saveCurrentEnergy(pendingCumulativeEnergy, pendingDateStr);
    
    pendingUploadFlag = false;
    uploadCompletedToday = true;

    preferences.begin("energy_store", false);
    preferences.putBool("pendFlag", false);
    preferences.putBool("completed", true);
    preferences.end();
    
    Serial.println(">> Success: Pending Daily Energy uploaded! <<");
}

/* --- DAILY SCHEDULE CHECKER --- */

void checkDailyUpload() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return;

    char dateBuffer[11];
    strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &timeinfo);
    String currentDateStr = String(dateBuffer);

    // 1. Midnight Reset - Reset flags if it's a new calendar day
    if (currentDateStr != lastUploadedDate) {
        uploadCompletedToday = false;
        preferences.begin("energy_store", false);
        preferences.putBool("completed", false);
        preferences.end();
    }
    if (currentDateStr != lastPredictedDate) {
        predictionCompletedToday = false;
        preferences.begin("energy_store", false);
        preferences.putBool("predCompleted", false);
        preferences.end();
    }

    // 2. 10:00 PM Trigger
    if (timeinfo.tm_hour >= 22) {
        
        // --- A. Handle Daily Energy ---
        if (!uploadCompletedToday && !pendingUploadFlag) {
            float dailyUsage = currentAccumulatedEnergy - savedPreviousEnergy;
            if (dailyUsage < 0) dailyUsage = 0.0; 

            char timeBuffer[20];
            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
            unsigned long long unixEpochMs = getUnixTimestampMs();

            if (WiFi.status() == WL_CONNECTED && app.ready()) {
                DynamicJsonDocument doc(256);
                doc["energy"] = dailyUsage;
                doc["cumulativeEnergy"] = currentAccumulatedEnergy;
                doc["timestamp"] = String(timeBuffer);
                doc["sendTimeMs"] = unixEpochMs;

                String jsonString;
                serializeJson(doc, jsonString);
                object_t jsonPayload(jsonString.c_str());

                Database.push<object_t>(client, energyPath, jsonPayload);
                saveCurrentEnergy(currentAccumulatedEnergy, currentDateStr);
                uploadCompletedToday = true;
                
                // Send to blockchain backend
                sendEnergyToBackend(dailyUsage);
                Serial.println(">> Success: Daily Energy Uploaded. <<");
            } else {
                savePendingUpload(dailyUsage, currentAccumulatedEnergy, currentDateStr);
                Serial.println(">> Offline: Daily Energy saved to NVS. <<");
            }
        }

        // --- B. Handle Daily Prediction ---
        if (!predictionCompletedToday && !pendingPredictionFlag) {
            if (WiFi.status() == WL_CONNECTED && app.ready()) {
                // Call the prediction function to calculate and upload immediately
                calculateAndUploadPrediction(timeinfo); 
            } else {
                // Offline: Calculate it now and save to NVS to upload later
                preferences.begin("energy_store", false);
                int currentDayIndex = preferences.getInt("regDay", 32);
                
                float predictedEnergy = 25.5494 + (-0.0758 * (float)currentDayIndex);
                if (predictedEnergy < 0) predictedEnergy = 0.0;

                pendingPredictionFlag = true;
                pendingPredDayIndex = currentDayIndex;
                pendingPredEnergy = predictedEnergy;

                preferences.putBool("pendPredFlag", true);
                preferences.putInt("pendPredDay", currentDayIndex);
                preferences.putFloat("pendPredEn", predictedEnergy);
                preferences.end();
                
                Serial.println(">> Offline: Daily Prediction saved to NVS. <<");
            }
        }
    }
}