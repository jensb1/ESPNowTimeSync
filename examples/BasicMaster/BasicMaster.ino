/*
  Basic Master Example
  
  This example shows how to set up an ESP32 as a time sync master.
  The master provides time reference for client devices.
*/

#include <ESPNowTimeSync.h>

ESPNowTimeSync timeSync;

void setup() {
  Serial.begin(115200);
  
  // Initialize as master (no master MAC needed)
  if (!timeSync.begin(true)) {
    Serial.println("Failed to initialize TimeSync as master");
    while(1) delay(1000);
  }
  
  // Start synchronization
  timeSync.startSync();
  
  Serial.println("Master started successfully");
  Serial.printf("Local MAC: %s\n", WiFi.macAddress().c_str());
}

void loop() {
  // Master doesn't need to call update()
  delay(1000);
  
  // Optional: Print current time every 5 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 5000) {
    Serial.printf("Master time: %lld µs\n", timeSync.getSyncedTime());
    lastPrint = millis();
  }
} 