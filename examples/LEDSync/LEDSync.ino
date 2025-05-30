/*
  LED Synchronization Example
  
  This example demonstrates synchronized LED blinking between devices.
  Works as both master and client - set IS_MASTER accordingly.
  
  IMPORTANT: Update MASTER_MAC with your master's MAC address for clients!
*/

#include <ESPNowTimeSync.h>
#include <ESPNowTimeSyncLED.h>

// Configuration
#define IS_MASTER 1  // Set to 1 for master, 0 for client
#define LED_PIN 2    // Built-in LED pin (adjust if needed)

// UPDATE THIS WITH YOUR MASTER'S MAC ADDRESS (for clients)
const uint8_t MASTER_MAC[6] = {0xd8, 0x3b, 0xda, 0x42, 0xe6, 0x8c};

ESPNowTimeSync timeSync;
ESPNowTimeSyncLED ledSync(timeSync, LED_PIN);

void onSyncStatus(bool synchronized, int64_t offset) {
  if (synchronized) {
    Serial.printf("✓ Synchronized! Offset: %lld µs - LED sync active\n", offset);
  } else {
    Serial.println("✗ Lost synchronization - LED sync paused");
  }
}

void setup() {
  Serial.begin(115200);
  
  Serial.printf("Device MAC: %s\n", WiFi.macAddress().c_str());
  Serial.printf("Role: %s\n", IS_MASTER ? "MASTER" : "CLIENT");
  
  // Initialize time sync
  bool success;
  if (IS_MASTER) {
    success = timeSync.begin(true);  // Master
  } else {
    success = timeSync.begin(false, MASTER_MAC);  // Client
  }
  
  if (!success) {
    Serial.println("Failed to initialize TimeSync");
    while(1) delay(1000);
  }
  
  // Set callbacks
  timeSync.onSyncStatus(onSyncStatus);
  
  // Initialize LED sync
  ledSync.begin();
  ledSync.setPulseWidth(100);  // 100µs pulse width
  
  // Start synchronization
  timeSync.startSync();
  ledSync.start();
  
  Serial.println("Setup complete - LED will sync when time is synchronized");
}

void loop() {
  // Update time sync (important for clients)
  timeSync.update();
  
  // Print periodic status
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 5000) {
    if (timeSync.isSynchronized()) {
      auto stats = timeSync.getStats();
      Serial.printf("[%s] Time: %lld µs, Offset: %lld µs, Success: %.1f%%\n",
                   IS_MASTER ? "MASTER" : "CLIENT",
                   timeSync.getSyncedTime(), timeSync.getOffset(), stats.success_rate);
    } else {
      Serial.printf("[%s] Synchronizing...\n", IS_MASTER ? "MASTER" : "CLIENT");
    }
    lastStatus = millis();
  }
  
  delay(100);
} 