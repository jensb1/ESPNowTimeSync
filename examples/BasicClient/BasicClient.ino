/*
  Basic Client Example
  
  This example shows how to set up an ESP32 as a time sync client.
  The client synchronizes its time with a master device.
  
  IMPORTANT: Update MASTER_MAC with your master's MAC address!
*/

#include <ESPNowTimeSync.h>

// UPDATE THIS WITH YOUR MASTER'S MAC ADDRESS
const uint8_t MASTER_MAC[6] = {0xd8, 0x3b, 0xda, 0x42, 0xe6, 0x8c};

ESPNowTimeSync timeSync;

void onSyncStatus(bool synchronized, int64_t offset) {
  if (synchronized) {
    Serial.printf("✓ Synchronized! Offset: %lld µs\n", offset);
  } else {
    Serial.println("✗ Lost synchronization");
  }
}

void setup() {
  Serial.begin(115200);
  
  Serial.printf("Client MAC: %s\n", WiFi.macAddress().c_str());
  
  // Initialize as client with master MAC
  if (!timeSync.begin(false, MASTER_MAC)) {
    Serial.println("Failed to initialize TimeSync as client");
    while(1) delay(1000);
  }
  
  // Set sync status callback
  timeSync.onSyncStatus(onSyncStatus);
  
  // Start synchronization
  timeSync.startSync();
  
  Serial.println("Client started, syncing...");
}

void loop() {
  // Client must call update() regularly
  timeSync.update();
  
  // Print stats every 10 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 10000) {
    if (timeSync.isSynchronized()) {
      auto stats = timeSync.getStats();
      Serial.printf("Synced time: %lld µs, Success rate: %.1f%%, Offset: %lld µs\n",
                   timeSync.getSyncedTime(), stats.success_rate, timeSync.getOffset());
    }
    lastPrint = millis();
  }
  
  delay(100);
} 