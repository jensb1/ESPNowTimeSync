#ifndef ESPNOW_TIMESYNC_H
#define ESPNOW_TIMESYNC_H

#include <esp_now.h>
#include <WiFi.h>
#include <esp_timer.h>
#include <functional>

// Forward declarations
class ESPNowTimeSync;

// Callback function types
typedef std::function<void(bool, int64_t)> SyncStatusCallback;
typedef std::function<void(int64_t)> SyncEventCallback;

// Configuration structure
struct TimeSyncConfig {
  uint8_t channel = 0;
  float smoothing_alpha = 0.05f;
  uint32_t sync_interval_ms = 1000;
  uint32_t response_timeout_ms = 50;
  bool enable_logging = false;
  uint32_t log_interval_syncs = 10;
  // When not synchronized, retry faster to reacquire quickly
  uint32_t resync_interval_ms = 200;
  // After this many consecutive missed responses, consider unsynchronized
  uint32_t max_missed_responses = 3;
};

// Statistics structure
struct SyncStats {
  uint32_t sync_count = 0;
  uint32_t fail_count = 0;
  int64_t last_offset_us = 0;
  int64_t last_rtt_us = 0;
  float success_rate = 0.0f;
};

class ESPNowTimeSync {
public:
  // Constructor
  ESPNowTimeSync();
  ~ESPNowTimeSync();

  // Initialization
  bool begin(bool is_master, const uint8_t* master_mac = nullptr);
  bool begin(bool is_master, const uint8_t* master_mac, const TimeSyncConfig& config);
  
  // Configuration
  void setConfig(const TimeSyncConfig& config);
  TimeSyncConfig getConfig() const { return config_; }
  
  // Callbacks
  void onSyncStatus(SyncStatusCallback callback) { sync_status_callback_ = callback; }
  void onSyncEvent(SyncEventCallback callback) { sync_event_callback_ = callback; }
  
  // Time functions
  int64_t getSyncedTime() const;
  int64_t getOffset() const { return current_offset_; }
  bool isSynchronized() const { return is_synchronized_; }
  
  // Statistics
  SyncStats getStats() const { return stats_; }
  void resetStats();
  
  // Control
  void startSync();
  void stopSync();
  bool isRunning() const { return sync_active_; }
  
  // Update function (call in loop for client mode)
  void update();

private:
  // Internal structures
  struct alignas(4) TimeRequest {
    uint64_t t1;
  };
  
  struct alignas(4) TimeResponse {
    uint64_t t1;
    uint64_t t2_recv;
    uint64_t t3_send; 
  };

  // Configuration and state
  TimeSyncConfig config_;
  bool is_master_;
  bool initialized_;
  bool sync_active_;
  bool is_synchronized_;
  uint8_t master_mac_[6];
  
  // Timing variables
  int64_t current_offset_;
  float smoothed_offset_;
  uint32_t last_sync_time_;
  uint32_t consecutive_failures_;
  
  // Response handling (client only)
  volatile bool response_ready_;
  TimeResponse response_;
  uint64_t t4_;
  
  // Statistics
  SyncStats stats_;
  
  // Callbacks
  SyncStatusCallback sync_status_callback_;
  SyncEventCallback sync_event_callback_;
  
  // Internal methods
  void registerPeer(const uint8_t* mac);
  void processTimeRequest(const esp_now_recv_info_t* info, const uint8_t* data, int len);
  void processTimeResponse(const esp_now_recv_info_t* info, const uint8_t* data, int len);
  void performSync();
  void updateSyncStatus(bool synchronized);
  void logStats();
  
  // Static callback wrappers
  static void onReceiveWrapper(const esp_now_recv_info_t* info, const uint8_t* data, int len);
  static ESPNowTimeSync* instance_;
};

#endif // ESPNOW_TIMESYNC_H 
