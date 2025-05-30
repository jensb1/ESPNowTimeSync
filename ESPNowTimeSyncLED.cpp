#include "ESPNowTimeSyncLED.h"

ESPNowTimeSyncLED::ESPNowTimeSyncLED(ESPNowTimeSync& timeSync, int ledPin)
  : time_sync_(timeSync)
  , led_pin_(ledPin)
  , pulse_width_us_(500)
  , interval_us_(1000000)  // 1 second
  , active_(false)
  , initialized_(false)
  , pulse_timer_(nullptr)
  , off_timer_(nullptr) {
}

ESPNowTimeSyncLED::~ESPNowTimeSyncLED() {
  stop();
  if (pulse_timer_) {
    esp_timer_delete(pulse_timer_);
  }
  if (off_timer_) {
    esp_timer_delete(off_timer_);
  }
}

void ESPNowTimeSyncLED::begin() {
  if (initialized_) return;
  
  pinMode(led_pin_, OUTPUT);
  digitalWrite(led_pin_, LOW);
  
  // Create timers
  esp_timer_create_args_t pulse_cfg = {
    .callback = &onPulseTimer,
    .arg = this,
    .name = "led_pulse"
  };
  esp_timer_create(&pulse_cfg, &pulse_timer_);
  
  esp_timer_create_args_t off_cfg = {
    .callback = &onOffTimer,
    .arg = this,
    .name = "led_off"
  };
  esp_timer_create(&off_cfg, &off_timer_);
  
  // Register sync status callback
  time_sync_.onSyncStatus([this](bool sync, int64_t offset) {
    this->onSyncStatus(sync, offset);
  });
  
  initialized_ = true;
}

void ESPNowTimeSyncLED::start() {
  if (!initialized_ || active_) return;
  
  active_ = true;
  
  // Start immediately if already synchronized
  if (time_sync_.isSynchronized()) {
    scheduleNextPulse();
  }
}

void ESPNowTimeSyncLED::stop() {
  if (!active_) return;
  
  active_ = false;
  digitalWrite(led_pin_, LOW);
  
  if (pulse_timer_) {
    esp_timer_stop(pulse_timer_);
  }
  if (off_timer_) {
    esp_timer_stop(off_timer_);
  }
}

void ESPNowTimeSyncLED::scheduleNextPulse() {
  if (!active_ || !time_sync_.isSynchronized()) return;
  
  int64_t now_us = esp_timer_get_time();
  int64_t synced_us = now_us + time_sync_.getOffset();
  
  // Calculate next interval boundary
  int64_t current_interval = synced_us / interval_us_;
  int64_t next_interval = (current_interval + 1) * interval_us_;
  int64_t delay_us = next_interval - synced_us;
  
  // Ensure delay is positive
  if (delay_us <= 0) {
    delay_us += interval_us_;
  }
  
  esp_timer_start_once(pulse_timer_, delay_us);
}

void ESPNowTimeSyncLED::onSyncStatus(bool synchronized, int64_t offset) {
  if (synchronized && active_ && !esp_timer_is_active(pulse_timer_)) {
    scheduleNextPulse();
  }
}

void IRAM_ATTR ESPNowTimeSyncLED::onPulseTimer(void* arg) {
  ESPNowTimeSyncLED* led = static_cast<ESPNowTimeSyncLED*>(arg);
  
  // Turn on LED and schedule turn-off
  digitalWrite(led->led_pin_, HIGH);
  esp_timer_start_once(led->off_timer_, led->pulse_width_us_);
  
  // Schedule next pulse
  led->scheduleNextPulse();
}

void IRAM_ATTR ESPNowTimeSyncLED::onOffTimer(void* arg) {
  ESPNowTimeSyncLED* led = static_cast<ESPNowTimeSyncLED*>(arg);
  digitalWrite(led->led_pin_, LOW);
} 