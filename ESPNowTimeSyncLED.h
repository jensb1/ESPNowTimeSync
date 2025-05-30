#ifndef ESPNOW_TIMESYNC_LED_H
#define ESPNOW_TIMESYNC_LED_H

#include "ESPNowTimeSync.h"

class ESPNowTimeSyncLED {
public:
  ESPNowTimeSyncLED(ESPNowTimeSync& timeSync, int ledPin = 2);
  ~ESPNowTimeSyncLED();
  
  // Configuration
  void setPulseWidth(uint32_t pulse_us) { pulse_width_us_ = pulse_us; }
  void setInterval(uint32_t interval_us) { interval_us_ = interval_us; }
  
  // Control
  void begin();
  void start();
  void stop();
  bool isRunning() const { return active_; }

private:
  ESPNowTimeSync& time_sync_;
  int led_pin_;
  uint32_t pulse_width_us_;
  uint32_t interval_us_;
  bool active_;
  bool initialized_;
  
  // Timer handles
  esp_timer_handle_t pulse_timer_;
  esp_timer_handle_t off_timer_;
  
  // Internal methods
  void scheduleNextPulse();
  void onSyncStatus(bool synchronized, int64_t offset);
  
  // Timer callbacks
  static void IRAM_ATTR onPulseTimer(void* arg);
  static void IRAM_ATTR onOffTimer(void* arg);
};

#endif // ESPNOW_TIMESYNC_LED_H 