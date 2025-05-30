#include "ESPNowTimeSync.h"

// Static instance pointer for callback
ESPNowTimeSync* ESPNowTimeSync::instance_ = nullptr;

ESPNowTimeSync::ESPNowTimeSync() 
  : is_master_(false)
  , initialized_(false)
  , sync_active_(false)
  , is_synchronized_(false)
  , current_offset_(0)
  , smoothed_offset_(0.0f)
  , last_sync_time_(0)
  , response_ready_(false)
  , t4_(0)
  , sync_status_callback_(nullptr)
  , sync_event_callback_(nullptr) {
  
  // Set default MAC (should be overridden)
  memset(master_mac_, 0, 6);
  
  // Reset stats
  resetStats();
}

ESPNowTimeSync::~ESPNowTimeSync() {
  if (initialized_) {
    esp_now_deinit();
  }
}

bool ESPNowTimeSync::begin(bool is_master, const uint8_t* master_mac) {
  TimeSyncConfig default_config;
  return begin(is_master, master_mac, default_config);
}

bool ESPNowTimeSync::begin(bool is_master, const uint8_t* master_mac, const TimeSyncConfig& config) {
  if (initialized_) {
    return false;
  }
  
  is_master_ = is_master;
  config_ = config;
  
  // Store master MAC for client mode
  if (!is_master_ && master_mac != nullptr) {
    memcpy(master_mac_, master_mac, 6);
  }
  
  // Handle WiFi initialization
  WiFiMode_t current_mode = WiFi.getMode();
  if (current_mode == WIFI_OFF) {
    // WiFi was off, initialize it
    WiFi.mode(WIFI_STA);
  } else if (current_mode == WIFI_AP) {
    // If in AP mode, switch to AP+STA mode to maintain AP functionality
    WiFi.mode(WIFI_AP_STA);
  } else if (current_mode != WIFI_STA && current_mode != WIFI_AP_STA) {
    // For any other mode, switch to STA mode
    WiFi.mode(WIFI_STA);
  }
  
  // Disconnect from any existing AP
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
  }
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    if (config_.enable_logging) {
      Serial.println("[TimeSync] ESP-NOW init failed");
    }
    return false;
  }
  
  // Set static instance for callbacks
  instance_ = this;
  
  // Register receive callback
  esp_now_register_recv_cb(onReceiveWrapper);
  
  // Add master peer for client mode
  if (!is_master_) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, master_mac_, 6);
    peer.channel = config_.channel;
    peer.encrypt = false;
    if (esp_now_add_peer(&peer) != ESP_OK) {
      if (config_.enable_logging) {
        Serial.println("[TimeSync] Failed to add master peer");
      }
      return false;
    }
  }
  
  initialized_ = true;
  
  if (config_.enable_logging) {
    Serial.printf("[TimeSync] Initialized as %s\n", is_master_ ? "MASTER" : "CLIENT");
    if (!is_master_) {
      Serial.printf("[TimeSync] Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
        master_mac_[0], master_mac_[1], master_mac_[2], 
        master_mac_[3], master_mac_[4], master_mac_[5]);
    }
  }
  
  return true;
}

void ESPNowTimeSync::setConfig(const TimeSyncConfig& config) {
  config_ = config;
}

int64_t ESPNowTimeSync::getSyncedTime() const {
  return esp_timer_get_time() + current_offset_;
}

void ESPNowTimeSync::resetStats() {
  stats_.sync_count = 0;
  stats_.fail_count = 0;
  stats_.last_offset_us = 0;
  stats_.last_rtt_us = 0;
  stats_.success_rate = 0.0f;
}

void ESPNowTimeSync::startSync() {
  if (!initialized_) return;
  
  sync_active_ = true;
  last_sync_time_ = millis();
  
  if (is_master_) {
    // Master is always synchronized to its own clock
    current_offset_ = 0;
    updateSyncStatus(true);
  }
  
  if (config_.enable_logging) {
    Serial.printf("[TimeSync] Sync started (%s)\n", is_master_ ? "MASTER" : "CLIENT");
  }
}

void ESPNowTimeSync::stopSync() {
  sync_active_ = false;
  updateSyncStatus(false);
  
  if (config_.enable_logging) {
    Serial.println("[TimeSync] Sync stopped");
  }
}

void ESPNowTimeSync::update() {
  if (!initialized_ || !sync_active_) return;
  
  // Only clients need to actively sync
  if (!is_master_ && (millis() - last_sync_time_) >= config_.sync_interval_ms) {
    performSync();
    last_sync_time_ = millis();
  }
}

void ESPNowTimeSync::registerPeer(const uint8_t* mac) {
  if (!is_master_) return;
  
  // Check if peer already exists
  if (esp_now_is_peer_exist(mac)) {
    return;
  }
  
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = config_.channel;
  peer.encrypt = false;
  
  if (esp_now_add_peer(&peer) != ESP_OK && config_.enable_logging) {
    Serial.printf("[TimeSync] Could not add peer %02X:%02X:%02X:%02X:%02X:%02X\n",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }
}

void ESPNowTimeSync::processTimeRequest(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (!is_master_ || len != sizeof(TimeRequest)) return;
  
  uint64_t t2_recv = esp_timer_get_time();
  registerPeer(info->src_addr);
  
  TimeRequest req;
  memcpy(&req, data, sizeof(req));
  
  TimeResponse res;
  res.t1 = req.t1;
  res.t2_recv = t2_recv;
  res.t3_send = esp_timer_get_time();
  
  esp_now_send(info->src_addr, reinterpret_cast<const uint8_t*>(&res), sizeof(res));
}

void ESPNowTimeSync::processTimeResponse(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (is_master_ || len != sizeof(TimeResponse)) return;
  
  t4_ = esp_timer_get_time();
  memcpy(&response_, data, sizeof(response_));
  response_ready_ = true;
}

void ESPNowTimeSync::performSync() {
  if (is_master_) return;
  
  // Send time request
  response_ready_ = false;
  uint64_t t1 = esp_timer_get_time();
  TimeRequest req{ .t1 = t1 };
  
  esp_now_send(master_mac_, reinterpret_cast<const uint8_t*>(&req), sizeof(req));
  
  // Wait for response
  uint32_t start = millis();
  while (!response_ready_ && (millis() - start) < config_.response_timeout_ms) {
    delayMicroseconds(100);
  }
  
  if (response_ready_) {
    stats_.sync_count++;
    
    // Calculate timing metrics
    int64_t roundTrip = t4_ - response_.t1;
    int64_t masterProcessing = response_.t3_send - response_.t2_recv;
    int64_t networkDelay = (roundTrip - masterProcessing) / 2;
    int64_t offset = response_.t2_recv + networkDelay - response_.t1;
    
    // Apply smoothing
    if (stats_.sync_count == 1) {
      smoothed_offset_ = offset;
    } else {
      smoothed_offset_ = (1 - config_.smoothing_alpha) * smoothed_offset_ + 
                        config_.smoothing_alpha * offset;
    }
    
    current_offset_ = (int64_t)smoothed_offset_;
    
    // Update statistics
    stats_.last_offset_us = offset;
    stats_.last_rtt_us = roundTrip;
    stats_.success_rate = 100.0f * stats_.sync_count / (stats_.sync_count + stats_.fail_count);
    
    // Check if synchronized (after a few successful syncs)
    bool was_synchronized = is_synchronized_;
    is_synchronized_ = (stats_.sync_count >= 3);
    
    if (is_synchronized_ != was_synchronized) {
      updateSyncStatus(is_synchronized_);
    }
    
    // Trigger sync event callback
    if (sync_event_callback_ && is_synchronized_) {
      sync_event_callback_(getSyncedTime());
    }
    
    // Log statistics
    if (config_.enable_logging && (stats_.sync_count % config_.log_interval_syncs == 0)) {
      logStats();
    }
    
  } else {
    stats_.fail_count++;
    stats_.success_rate = 100.0f * stats_.sync_count / (stats_.sync_count + stats_.fail_count);
    
    if (config_.enable_logging) {
      Serial.printf("[TimeSync] Timeout #%lu (success rate: %.1f%%)\n", 
                   stats_.fail_count, stats_.success_rate);
    }
  }
}

void ESPNowTimeSync::updateSyncStatus(bool synchronized) {
  is_synchronized_ = synchronized;
  if (sync_status_callback_) {
    sync_status_callback_(synchronized, current_offset_);
  }
}

void ESPNowTimeSync::logStats() {
  if (!config_.enable_logging) return;
  
  Serial.printf("[TimeSync] Sync #%lu: offset=%lld µs, smoothed=%.1f µs, RTT=%lld µs, "
               "success=%.1f%%, syncTime=%lld µs\n",
               stats_.sync_count, stats_.last_offset_us, smoothed_offset_, 
               stats_.last_rtt_us, stats_.success_rate, getSyncedTime());
}

void ESPNowTimeSync::onReceiveWrapper(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (instance_) {
    if (instance_->is_master_) {
      instance_->processTimeRequest(info, data, len);
    } else {
      instance_->processTimeResponse(info, data, len);
    }
  }
} 