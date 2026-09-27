// void checkRelayState() {
//     if (!app.ready()) return;

//     String state = Database.get<String>(client, relayPath);
    
//     // Interpret the Firebase state and switch the relay
//     if (state == "ON" || state == "true" || state == "1") {
//         digitalWrite(RELAY_PIN, HIGH);
//         Serial.println("Relay turned ON via Firebase");
//     } else if (state == "OFF" || state == "false" || state == "0") {
//         digitalWrite(RELAY_PIN, LOW);
//         Serial.println("Relay turned OFF via Firebase");
//     }
// }




void checkRelayState() {
    if (!app.ready()) return;

    String state = Database.get<String>(client, relayPath);
    
    // Interpret the Firebase state and switch the relay (Reversed Logic for Active-Low)
    if (state == "ON" || state == "true" || state == "1") {
        digitalWrite(RELAY_PIN, LOW); // Changed to LOW to turn the relay ON
        Serial.println("Relay turned ON via Firebase");
    } else if (state == "OFF" || state == "false" || state == "0") {
        digitalWrite(RELAY_PIN, HIGH); // Changed to HIGH to turn the relay OFF
        Serial.println("Relay turned OFF via Firebase");
    }
}