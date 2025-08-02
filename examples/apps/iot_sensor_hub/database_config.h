// IoT Sensor Hub Database Configuration
// Multi-sensor data logging and device management system
#pragma once

#include "../engine/database/partitioned_system.h"

// IoT-specific safe database partition configuration (12.75KB total, 3.25KB safety margin)
#define ROM_PARTITION_SIZE   2048    // 2KB ROM - sensor definitions, calibration (compressed)
#define SAVE_PARTITION_SIZE  5120    // 5KB save - sensor readings, device states, config
#define BACKUP_PARTITION_SIZE 1536   // 1.5KB backup - critical device config and network settings
#define RUNTIME_PARTITION_SIZE 4352  // 4.25KB runtime - reading cache, calculation buffers

// Memory safety validation
static_assert((ROM_PARTITION_SIZE + SAVE_PARTITION_SIZE + 
               BACKUP_PARTITION_SIZE + RUNTIME_PARTITION_SIZE) <= 13056,
              "IoT DB exceeds safe 12.75KB limit!");

// Memory usage: ROM=2KB, Save=5KB, Backup=1.5KB, Runtime=4.25KB = 12.75KB total (80% usage)

// IoT-specific namespaces
#define NS_SENSORS        0x20      // Sensor definitions and configuration
#define NS_READINGS       0x21      // Sensor reading data and history
#define NS_DEVICES        0x22      // Connected device states and control
#define NS_NETWORK        0x23      // Network and connectivity configuration
#define NS_AUTOMATION     0x24      // Automation rules and schedules

// IoT-specific categories
#define CAT_SENSOR_DEFS   0x01      // Sensor type definitions and setup
#define CAT_CALIBRATION   0x02      // Calibration data and offsets
#define CAT_THRESHOLDS    0x03      // Alert thresholds and limits
#define CAT_CURRENT       0x01      // Current readings and states
#define CAT_HISTORY       0x02      // Historical data (compressed)
#define CAT_STATISTICS    0x03      // Min/max/avg statistics
#define CAT_WIFI_CONFIG   0x01      // WiFi configuration
#define CAT_MQTT_CONFIG   0x02      // MQTT broker settings
#define CAT_RULES         0x01      // Automation rules
#define CAT_SCHEDULES     0x02      // Scheduled actions

// IoT-specific entry types
#define ENTRY_SENSOR_READING  0x90  // Timestamped sensor reading
#define ENTRY_DEVICE_STATE    0x91  // Device state snapshot
#define ENTRY_AUTOMATION_RULE 0x92  // Automation rule definition
#define ENTRY_NETWORK_CONFIG  0x93  // Network configuration

// Key generation macros for IoT data
#define SENSOR_DEF_KEY(id)        MAKE_KEY(NS_SENSORS, CAT_SENSOR_DEFS, id)
#define SENSOR_READING_KEY(id)    MAKE_KEY(NS_READINGS, CAT_CURRENT, id)
#define SENSOR_HISTORY_KEY(id, timestamp) MAKE_KEY(NS_READINGS, CAT_HISTORY, ((id << 8) | (timestamp & 0xFF)))
#define DEVICE_STATE_KEY(id)      MAKE_KEY(NS_DEVICES, CAT_CURRENT, id)
#define AUTOMATION_RULE_KEY(id)   MAKE_KEY(NS_AUTOMATION, CAT_RULES, id)
#define WIFI_CONFIG_KEY           MAKE_KEY(NS_NETWORK, CAT_WIFI_CONFIG, 1)
#define MQTT_CONFIG_KEY           MAKE_KEY(NS_NETWORK, CAT_MQTT_CONFIG, 1)

// IoT data structures (memory optimized)
struct SensorDefinition {
    uint8_t id;                 // Sensor ID (1-255)
    uint8_t type;               // temperature=1, humidity=2, pressure=3, etc.
    uint8_t unit;               // celsius=1, fahrenheit=2, percent=3, etc.
    uint8_t pin;                // GPIO pin number
    uint16_t sampleInterval;    // ms between readings
    int16_t calibrationOffset;  // Calibration adjustment (scaled)
    uint8_t precision;          // Decimal places (0-3)
    char name[8];               // Short sensor name
} __attribute__((packed));

struct SensorReading {
    uint16_t timestamp;         // Relative timestamp (minutes since boot)
    uint8_t sensorId;           // Which sensor
    int16_t value;              // The reading (scaled for precision)
    uint8_t quality;            // Reading quality (0-100)
} __attribute__((packed));

struct DeviceState {
    uint8_t deviceId;           // Device ID
    uint8_t deviceType;         // relay=1, led=2, servo=3, etc.
    uint8_t state;              // on/off, position, etc.
    uint8_t pin;                // GPIO pin
    uint16_t value;             // Current value (PWM, position, etc.)
    uint16_t lastUpdated;       // When state was last changed (minutes)
} __attribute__((packed));

struct AutomationRule {
    uint8_t ruleId;             // Rule ID
    uint8_t triggerType;        // sensor_threshold=1, time=2, manual=3
    uint8_t triggerSensor;      // Sensor ID (if sensor trigger)
    int16_t triggerValue;       // Threshold value (scaled)
    uint8_t comparison;         // greater=1, less=2, equal=3
    uint8_t targetDevice;       // Device to control
    uint8_t targetAction;       // Action to take
    uint16_t targetValue;       // Value to set
    bool enabled;               // Rule active
} __attribute__((packed));

struct WiFiConfig {
    char ssid[24];              // WiFi SSID (shortened)
    char password[24];          // WiFi password (shortened)
    bool dhcp;                  // Use DHCP
    uint32_t staticIP;          // Static IP if not DHCP
} __attribute__((packed));

struct MQTTConfig {
    char broker[32];            // MQTT broker address
    uint16_t port;              // MQTT port
    char username[16];          // MQTT username
    char password[16];          // MQTT password
    char topicPrefix[16];       // Topic prefix
    uint16_t keepAlive;         // Keep alive interval
} __attribute__((packed));

// IoT configuration (memory-safe, 80% LP-SRAM usage)
static const PartitionConfig IOT_CONFIG = {
    .romSize = ROM_PARTITION_SIZE,
    .saveSize = SAVE_PARTITION_SIZE,
    .backupSize = BACKUP_PARTITION_SIZE,
    .runtimeSize = RUNTIME_PARTITION_SIZE,
    .enableCompression = true,      // Essential for sensor history data
    .enableEncryption = true,       // Protect network credentials
    .maxCacheEntries = 32,          // Reasonable cache for sensor readings
    .safetyLevel = 1               // Standard bounds checking
};

// IoT-specific convenience macros
#define IOT_DB_INIT() database.initialize(&IOT_CONFIG)

#define IOT_DEFINE_SENSOR(id, type, pin, interval, name) do { \
    SensorDefinition def = {id, type, 1, pin, interval, 0, 1}; \
    strncpy(def.name, name, 7); \
    def.name[7] = '\0'; \
    database.set(SENSOR_DEF_KEY(id), &def, sizeof(def), ENTRY_STRUCT); \
} while(0)

#define IOT_LOG_READING(sensorId, value, quality) do { \
    SensorReading reading = {(uint16_t)(millis()/60000), sensorId, (int16_t)(value * 100), quality}; \
    database.set(SENSOR_READING_KEY(sensorId), &reading, sizeof(reading), ENTRY_SENSOR_READING); \
} while(0)

#define IOT_GET_CURRENT_READING(sensorId) ({ \
    SensorReading reading = {}; \
    database.get(SENSOR_READING_KEY(sensorId), &reading, sizeof(reading)); \
    reading; \
})

#define IOT_SET_DEVICE_STATE(deviceId, state, value) do { \
    DeviceState dev = {deviceId, 1, state, 0, value, (uint16_t)(millis()/60000)}; \
    database.set(DEVICE_STATE_KEY(deviceId), &dev, sizeof(dev), ENTRY_DEVICE_STATE); \
} while(0)

#define IOT_GET_DEVICE_STATE(deviceId) ({ \
    DeviceState dev = {}; \
    database.get(DEVICE_STATE_KEY(deviceId), &dev, sizeof(dev)); \
    dev; \
})

#define IOT_CREATE_RULE(ruleId, sensorId, threshold, deviceId, action) do { \
    AutomationRule rule = {ruleId, 1, sensorId, (int16_t)(threshold * 100), 1, deviceId, action, 0, true}; \
    database.set(AUTOMATION_RULE_KEY(ruleId), &rule, sizeof(rule), ENTRY_AUTOMATION_RULE); \
} while(0)

#define IOT_SET_WIFI_CONFIG(ssid, pass) do { \
    WiFiConfig wifi = {}; \
    strncpy(wifi.ssid, ssid, 23); \
    strncpy(wifi.password, pass, 23); \
    wifi.dhcp = true; \
    database.set(WIFI_CONFIG_KEY, &wifi, sizeof(wifi), ENTRY_NETWORK_CONFIG); \
} while(0)

#define IOT_GET_WIFI_CONFIG() ({ \
    WiFiConfig wifi = {}; \
    database.get(WIFI_CONFIG_KEY, &wifi, sizeof(wifi)); \
    wifi; \
})

// Sensor type constants
#define SENSOR_TEMPERATURE    1
#define SENSOR_HUMIDITY       2
#define SENSOR_PRESSURE       3
#define SENSOR_LIGHT          4
#define SENSOR_MOTION         5
#define SENSOR_SOUND          6
#define SENSOR_CO2            7
#define SENSOR_TVOC           8
#define SENSOR_PM25           9
#define SENSOR_VOLTAGE        10

// Device type constants
#define DEVICE_RELAY          1
#define DEVICE_LED            2
#define DEVICE_SERVO          3
#define DEVICE_FAN            4
#define DEVICE_HEATER         5
#define DEVICE_PUMP           6

// Memory usage analysis for IoT app:
// ROM: ~1.5KB (sensor definitions, calibration data, device configs)
// Save: ~3KB (current readings, device states, network config, rules)
// Backup: ~800B (critical config backup, network credentials)
// Runtime: ~2KB (reading cache, calculation buffers, temporary data)
// Total: ~7.3KB actual usage out of 12.75KB allocated = efficient with room for expansion

// This configuration balances functionality with memory safety,
// supporting comprehensive IoT operations within 80% of LP-SRAM
