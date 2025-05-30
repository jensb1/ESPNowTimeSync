/*
  Advanced Configuration Example
  
  This example shows how to use custom configuration and callbacks
  for fine-tuning synchronization behavior.
*/

#include <ESPNowTimeSync.h>

// UPDATE THIS WITH YOUR MASTER'S MAC ADDRESS (for clients)
const uint8_t MASTER_MAC[6] = {0xd8, 0x3b, 0xda, 0x42, 0xe6, 0x8c};

#define IS_MASTER 0  // Set to 1 for master, 0 for client

ESPNowTimeSync timeSync;

void onSyncStatus(bool synchronized, int64_t offset) {
  Serial.printf("Sync status changed: %s, offset: %lld µs\n", 
                synchronized ? "SYNCHRONIZED" : "NOT_SYNCHRONIZED", offset);
}

void onSyncEvent(int64_t synced_time) {
  // This is called every time a successful sync occurs
  Serial.printf("Sync event: %lld µs\n", synced_time);
}

void setup() {
  Serial.begin(115200);
  
  // Custom configuration
  TimeSyncConfig config;
  config.channel = 1;                    // WiFi channel
  config.smoothing_alpha = 0.02f;        // Lower = more stable
  config.sync_interval_ms = 500;         // Sync every 500ms
  config.response_timeout_ms = 30;       // 30ms timeout
  config.enable_logging = true;          // Enable detailed logging
  config.log_interval_syncs = 5;         // Log every 5 syncs
  
  // Initialize with custom config
  bool success;
  if (IS_MASTER) {
    success = timeSync.begin(true, nullptr, config);
  } else {
    success = timeSync.begin(false, MASTER_MAC, config);
  }
  
  if (!success) {
    Serial.println("Failed to initialize TimeSync");
    while(1) delay(1000);
  }
  
  // Set callbacks
  timeSync.onSyncStatus(onSyncStatus);
  timeSync.onSyncEvent(onSyncEvent);
  
  // Start sync
  timeSync.startSync();
  
  Serial.printf("Advanced TimeSync started as %s\n", IS_MASTER ? "MASTER" : "CLIENT");
}

void loop() {
  timeSync.update();
  
  // Print detailed stats every 15 seconds
  static unsigned long lastStats = 0;
  if (millis() - lastStats > 15000) {
    auto stats = timeSync.getStats();
    Serial.println("=== Detailed Statistics ===");
    Serial.printf("Sync Count: %lu\n", stats.sync_count);
    Serial.printf("Fail Count: %lu\n", stats.fail_count);
    Serial.printf("Success Rate: %.2f%%\n", stats.success_rate);
    Serial.printf("Last Offset: %lld µs\n", stats.last_offset_us);
    Serial.printf("Last RTT: %lld µs\n", stats.last_rtt_us);
    Serial.printf("Current Offset: %lld µs\n", timeSync.getOffset());
    Serial.printf("Synchronized: %s\n", timeSync.isSynchronized() ? "YES" : "NO");
    Serial.printf("Synced Time: %lld µs\n", timeSync.getSyncedTime());
    Serial.println("========================");
    lastStats = millis();
  }
  
  delay(50);
} 