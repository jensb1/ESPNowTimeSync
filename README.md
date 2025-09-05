# ESPNow TimeSync Library

A high-precision time synchronization library for ESP32 devices using the ESP-NOW protocol. Achieve microsecond-level synchronization between multiple ESP32 devices without requiring WiFi infrastructure.

## Features

- **High Precision**: Microsecond-level time synchronization
- **No WiFi Required**: Uses ESP-NOW for direct device communication
- **Master-Client Architecture**: One master provides time reference for multiple clients
- **Automatic Peer Discovery**: Master automatically registers connecting clients
- **LED Synchronization**: Built-in support for synchronized LED blinking
- **Comprehensive Statistics**: Detailed sync performance metrics
- **Configurable Parameters**: Customize sync intervals, timeouts, and smoothing
- **Callback Support**: Event-driven notifications for sync status changes

## Installation

1. Download or clone this library
2. Place it in your Arduino libraries folder (`~/Arduino/libraries/ESPNowTimeSync/`)
3. Restart Arduino IDE
4. Include the library: `#include <ESPNowTimeSync.h>`

## Quick Start

### Master Device
```cpp
#include <ESPNowTimeSync.h>

ESPNowTimeSync timeSync;

void setup() {
  Serial.begin(115200);
  timeSync.begin(true);  // true = master
  timeSync.startSync();
  Serial.printf("Master MAC: %s\n", WiFi.macAddress().c_str());
}

void loop() {
  delay(1000);
}
```

### Client Device
```cpp
#include <ESPNowTimeSync.h>

// Replace with your master's MAC address
const uint8_t MASTER_MAC[6] = {0xd8, 0x3b, 0xda, 0x42, 0xe6, 0x8c};

ESPNowTimeSync timeSync;

void setup() {
  Serial.begin(115200);
  timeSync.begin(false, MASTER_MAC);  // false = client
  timeSync.startSync();
}

void loop() {
  timeSync.update();  // Important: call regularly for clients
  delay(100);
}
```

## LED Synchronization

```cpp
#include <ESPNowTimeSync.h>
#include <ESPNowTimeSyncLED.h>

ESPNowTimeSync timeSync;
ESPNowTimeSyncLED ledSync(timeSync, 2);  // Pin 2

void setup() {
  timeSync.begin(false, MASTER_MAC);
  ledSync.begin();
  
  timeSync.startSync();
  ledSync.start();
}

void loop() {
  timeSync.update();
  delay(100);
}
```

## API Reference

### ESPNowTimeSync Class

#### Initialization
- `bool begin(bool is_master, const uint8_t* master_mac = nullptr)`
- `bool begin(bool is_master, const uint8_t* master_mac, const TimeSyncConfig& config)`

#### Control
- `void startSync()` - Start synchronization process
- `void stopSync()` - Stop synchronization
- `void update()` - Update sync (call regularly for clients)

#### Time Functions
- `int64_t getSyncedTime()` - Get current synchronized time in microseconds
- `int64_t getOffset()` - Get current time offset from master
- `bool isSynchronized()` - Check if device is synchronized

#### Statistics
- `SyncStats getStats()` - Get detailed synchronization statistics
- `void resetStats()` - Reset statistics counters

#### Callbacks
- `void onSyncStatus(SyncStatusCallback callback)` - Sync status changes
- `void onSyncEvent(SyncEventCallback callback)` - Successful sync events

### ESPNowTimeSyncLED Class

#### Initialization
- `ESPNowTimeSyncLED(ESPNowTimeSync& timeSync, int ledPin = 2)`
- `void begin()` - Initialize LED synchronization

#### Configuration
- `void setPulseWidth(uint32_t pulse_us)` - Set LED pulse duration
- `void setInterval(uint32_t interval_us)` - Set blink interval

#### Control
- `void start()` - Start LED synchronization
- `void stop()` - Stop LED synchronization
- `bool isRunning()` - Check if LED sync is active

## Configuration

```cpp
TimeSyncConfig config;
config.channel = 0;                    // WiFi channel (0-13)
config.smoothing_alpha = 0.05f;        // Offset smoothing (0.01-0.5)
config.sync_interval_ms = 1000;        // Sync frequency
config.response_timeout_ms = 50;       // Response timeout
config.enable_logging = true;          // Debug output
config.log_interval_syncs = 10;        // Log every N syncs
config.resync_interval_ms = 200;       // Faster retry when unsynced
config.max_missed_responses = 3;       // Mark unsynced after N timeouts

timeSync.begin(false, MASTER_MAC, config);
```

## Performance

- **Typical Accuracy**: ±10-50 microseconds between devices
- **Sync Frequency**: 1 Hz (configurable)
- **Network Latency**: <5ms typical ESP-NOW round-trip
- **Startup Time**: 3-5 seconds to achieve stable synchronization

## Troubleshooting

### Common Issues

1. **Devices not syncing**
   - Verify master MAC address is correct
   - Check WiFi channel compatibility
   - Ensure devices are within ESP-NOW range (~200m)

2. **Poor sync accuracy**
   - Reduce `sync_interval_ms` for faster convergence
   - Adjust `smoothing_alpha` (lower = more stable)
   - Check for WiFi interference

3. **Frequent timeouts**
   - Increase `response_timeout_ms`
   - Check device placement and interference
   - Verify power supply stability
   - If the master was offline and comes back, the client now retries faster when unsynced (`resync_interval_ms`) and marks itself unsynchronized after `max_missed_responses` consecutive misses, enabling quick re-lock.

4. **Channel mismatch**
   - Set `config.channel` to a fixed channel used by all devices. The library sets the radio channel when `channel > 0`, improving reliability after restarts.

### Debug Output

Enable logging in configuration:
```cpp
config.enable_logging = true;
config.log_interval_syncs = 5;  // Log every 5 syncs
```

Sample output:
```
[TimeSync] Sync #50: offset=-23 µs, smoothed=-18.5 µs, RTT=1247 µs, success=98.0%, syncTime=1234567890 µs
```

## Examples

The library includes several examples:

- **BasicMaster**: Simple master setup
- **BasicClient**: Simple client setup  
- **LEDSync**: Synchronized LED blinking
- **AdvancedConfig**: Custom configuration and callbacks

## License

This library is released under the MIT License. See LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests. 
