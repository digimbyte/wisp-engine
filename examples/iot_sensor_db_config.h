// IoT Sensor Hub Database Configuration - Memory Optimized
// Balanced IoT application within safety limits
#pragma once

// IoT-specific safe database partition configuration (13KB total, 3KB safety margin)
#define WISP_DB_ROM_PARTITION_SIZE   2048    // 2KB ROM - sensor definitions (compressed)
#define WISP_DB_SAVE_PARTITION_SIZE  5120    // 5KB save - readings, device state
#define WISP_DB_BACKUP_PARTITION_SIZE 1536   // 1.5KB backup - critical config
#define WISP_DB_RUNTIME_PARTITION_SIZE 4352  // 4.25KB runtime - reading cache, buffers

// Memory safety validation
static_assert((WISP_DB_ROM_PARTITION_SIZE + WISP_DB_SAVE_PARTITION_SIZE + 
               WISP_DB_BACKUP_PARTITION_SIZE + WISP_DB_RUNTIME_PARTITION_SIZE) <= 13312,
              "IoT DB exceeds safe 13KB limit!");

// Memory usage: ROM=2KB, Save=5KB, Backup=1.5KB, Runtime=4.25KB = 12.75KB total (80% usage)

// IoT-specific namespaces
#define NS_SENSORS        0x20      // Sensor definitions and config
#define NS_READINGS       0x21      // Sensor reading data  
#define NS_DEVICES        0x22      // Connected device states
#define NS_NETWORK        0x23      // Network and connectivity
#define NS_AUTOMATION     0x24      // Automation rules and schedules

// IoT-specific categories
#define CAT_SENSOR_DEFS   0x01      // Sensor type definitions
#define CAT_CALIBRATION   0x02      // Calibration data
#define CAT_THRESHOLDS    0x03      // Alert thresholds
#define CAT_CURRENT       0x01      // Current readings
#define CAT_HISTORY       0x02      // Historical data
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
#define SENSOR_DEF_KEY(id)        WISP_KEY_MAKE(NS_SENSORS, CAT_SENSOR_DEFS, id)
#define SENSOR_READING_KEY(id)    WISP_KEY_MAKE(NS_READINGS, CAT_CURRENT, id)
#define SENSOR_HISTORY_KEY(id, timestamp) WISP_KEY_MAKE(NS_READINGS, CAT_HISTORY, ((id << 16) | (timestamp & 0xFFFF)))
#define DEVICE_STATE_KEY(id)      WISP_KEY_MAKE(NS_DEVICES, CAT_CURRENT, id)
#define AUTOMATION_RULE_KEY(id)   WISP_KEY_MAKE(NS_AUTOMATION, CAT_RULES, id)
#define WIFI_CONFIG_KEY           WISP_KEY_MAKE(NS_NETWORK, CAT_WIFI_CONFIG, 1)
#define MQTT_CONFIG_KEY           WISP_KEY_MAKE(NS_NETWORK, CAT_MQTT_CONFIG, 1)

// IoT data structures
struct SensorDefinition {
    uint16_t id;
    uint8_t type;               // temperature=1, humidity=2, pressure=3, etc.
    uint8_t unit;               // celsius=1, fahrenheit=2, percent=3, etc.
    uint8_t precision;          // decimal places
    uint8_t pin;                // GPIO pin number
    uint16_t sampleInterval;    // ms between readings
    float minValue, maxValue;   // Valid range
    float calibrationOffset;    // Calibration adjustment
    float calibrationScale;     // Calibration multiplier
    char name[16];              // Sensor name
} __attribute__((packed));

struct SensorReading {
    uint32_t timestamp;         // Unix timestamp
    uint16_t sensorId;          // Which sensor
    float value;                // The reading
    uint8_t quality;            // Reading quality (0-100)
    uint8_t flags;              // Alert flags, error conditions
} __attribute__((packed));

struct DeviceState {
    uint16_t deviceId;
    uint8_t deviceType;         // relay=1, led=2, servo=3, etc.
    uint8_t state;              // on/off, position, etc.
    uint32_t lastUpdated;       // When state was last changed
    uint8_t pin;                // GPIO pin
    uint16_t value;             // Current value (PWM, position, etc.)
    char name[16];              // Device name
} __attribute__((packed));

struct AutomationRule {
    uint16_t ruleId;
    uint8_t triggerType;        // sensor_threshold=1, time=2, manual=3
    uint16_t triggerSensor;     // Sensor ID (if sensor trigger)
    float triggerValue;         // Threshold value
    uint8_t comparison;         // greater=1, less=2, equal=3
    uint16_t targetDevice;      // Device to control
    uint8_t targetAction;       // Action to take
    uint16_t targetValue;       // Value to set
    bool enabled;               // Rule active
    uint32_t lastTriggered;     // Last execution time
} __attribute__((packed));

struct WiFiConfig {
    char ssid[32];
    char password[64];
    bool dhcp;
    uint32_t staticIP;
    uint32_t gateway;
    uint32_t subnet;
    uint32_t dns1, dns2;
} __attribute__((packed));

struct MQTTConfig {
    char broker[64];
    uint16_t port;
    char username[32];
    char password[32];
    char clientId[32];
    char topicPrefix[32];
    uint16_t keepAlive;
    bool retained;
} __attribute__((packed));

// IoT configuration (memory-safe, 80% LP-SRAM usage)
static const WispPartitionConfig IOT_DB_CONFIG = {
    .romSize = WISP_DB_ROM_PARTITION_SIZE,
    .saveSize = WISP_DB_SAVE_PARTITION_SIZE,
    .backupSize = WISP_DB_BACKUP_PARTITION_SIZE,
    .runtimeSize = WISP_DB_RUNTIME_PARTITION_SIZE,
    .enableCompression = true,      // Essential for sensor data
    .enableEncryption = true,       // Protect network credentials
    .maxCacheEntries = 32,          // Conservative cache size
    .safetyLevel = 1               // Standard bounds checking
};

// IoT-specific convenience macros
#define IOT_DB_INIT() wispDB.initialize(&IOT_DB_CONFIG)

#define IOT_DEFINE_SENSOR(id, type, name, pin, interval) do { \
    SensorDefinition def = {id, type, 1, 2, pin, interval, -50.0f, 100.0f, 0.0f, 1.0f}; \
    strncpy(def.name, name, 15); \
    def.name[15] = '\0'; \
    wispDB.set(SENSOR_DEF_KEY(id), &def, sizeof(def), ENTRY_STRUCT, WISP_DB_PARTITION_ROM, FLAG_READ_ONLY); \
} while(0)

#define IOT_LOG_READING(sensorId, value, quality) do { \
    SensorReading reading = {millis()/1000, sensorId, value, quality, 0}; \
    wispDB.set(SENSOR_READING_KEY(sensorId), &reading, sizeof(reading), ENTRY_SENSOR_READING); \
} while(0)

#define IOT_GET_CURRENT_READING(sensorId) ({ \
    SensorReading reading = {}; \
    wispDB.get(SENSOR_READING_KEY(sensorId), &reading, sizeof(reading)); \
    reading; \
})

#define IOT_SET_DEVICE_STATE(deviceId, state, value) do { \
    DeviceState dev = {deviceId, 1, state, millis()/1000, 0, value}; \
    wispDB.set(DEVICE_STATE_KEY(deviceId), &dev, sizeof(dev), ENTRY_DEVICE_STATE); \
} while(0)

#define IOT_GET_DEVICE_STATE(deviceId) ({ \
    DeviceState dev = {}; \
    wispDB.get(DEVICE_STATE_KEY(deviceId), &dev, sizeof(dev)); \
    dev; \
})

#define IOT_CREATE_RULE(ruleId, sensorId, threshold, deviceId, action) do { \
    AutomationRule rule = {ruleId, 1, sensorId, threshold, 1, deviceId, action, 0, true, 0}; \
    wispDB.set(AUTOMATION_RULE_KEY(ruleId), &rule, sizeof(rule), ENTRY_AUTOMATION_RULE); \
} while(0)

#define IOT_SET_WIFI_CONFIG(ssid, pass) do { \
    WiFiConfig wifi = {}; \
    strncpy(wifi.ssid, ssid, 31); \
    strncpy(wifi.password, pass, 63); \
    wifi.dhcp = true; \
    wispDB.set(WIFI_CONFIG_KEY, &wifi, sizeof(wifi), ENTRY_NETWORK_CONFIG, WISP_DB_PARTITION_SAVE, FLAG_ENCRYPTED); \
} while(0)

#define IOT_GET_WIFI_CONFIG() ({ \
    WiFiConfig wifi = {}; \
    wispDB.get(WIFI_CONFIG_KEY, &wifi, sizeof(wifi)); \
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

// ROM data for IoT sensors (preloaded sensor definitions)
const uint8_t IOT_ROM_DATA[] = {
    // ROM header
    0x49, 0x4F, 0x54, 0x01,  // 'IOT' + version 1
    0x03, 0x00,              // 3 sensor definitions
    0x02, 0x00,              // 2 device definitions
    
    // Temperature sensor definition
    // SensorDefinition: id=1, type=TEMPERATURE, unit=celsius, precision=2, pin=34, interval=5000ms
    0x01, 0x00,              // id: 1
    0x01,                    // type: temperature
    0x01,                    // unit: celsius
    0x02,                    // precision: 2 decimal places
    0x22,                    // pin: 34
    0x88, 0x13,              // interval: 5000ms
    0x00, 0x00, 0x48, 0xC2,  // minValue: -50.0
    0x00, 0x00, 0xC8, 0x42,  // maxValue: 100.0
    0x00, 0x00, 0x00, 0x00,  // calibrationOffset: 0.0
    0x00, 0x00, 0x80, 0x3F,  // calibrationScale: 1.0
    'T', 'e', 'm', 'p', ' ', 'S', 'e', 'n', 's', 'o', 'r', 0, 0, 0, 0, 0,
    
    // Add more sensor definitions...
};

// Memory usage for IoT app:
// ROM: ~2KB (sensor/device definitions, calibration data)
// Save: ~3KB (current readings, device states, network config)
// Backup: ~1KB (critical config backup)
// Runtime: ~2KB (reading cache, buffers)
// Total: ~8KB out of 16KB = 50% used for comprehensive IoT hub
