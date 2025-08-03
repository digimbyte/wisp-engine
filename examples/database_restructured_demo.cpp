#include "../engine/database/partitioned_system.h"

// Include different app configurations
#include "apps/pokemon_rpg/database_config.h"
#include "apps/snake_game/database_config.h"
#include "apps/iot_sensor_hub/database_config.h"

#include "esp_log.h"

void demonstratePokemonConfig() {
    ESP_LOGI("DEMO", "\n=== Pokemon RPG Database Demo ===");
    
    // Initialize with Pokemon configuration
    ErrorCode result = database.initialize(&POKEMON_CONFIG);
    if (result != SUCCESS) {
        ESP_LOGE("DEMO", "Failed to initialize Pokemon database: %d", (int)result);
        return;
    }
    
    // Add some Pokemon species data
    POKEMON_DEFINE_SPECIES(25, "Pikachu", TYPE_ELECTRIC, 35, 55, 40, 50, 50, 90);
    POKEMON_DEFINE_SPECIES(6, "Charizard", TYPE_FIRE, 78, 84, 78, 109, 85, 100);
    
    // Create a trainer
    TrainerData trainer = {1, "Ash", 0, 0, 1000, {1, 2, 0, 0, 0, 0}};
    database.set(TRAINER_KEY(1), &trainer, sizeof(trainer), ENTRY_TRAINER);
    
    // Add some captured Pokemon
    PokemonInstance pikachu = {25, 25, 100, 1000, {1, 2, 3, 4}, {31, 31, 31, 31, 31, 31}, 1, "Pikachu"};
    PokemonInstance charizard = {6, 55, 250, 15000, {5, 6, 7, 8}, {31, 31, 31, 31, 31, 31}, 2, "Charizard"};
    
    POKEMON_CAPTURE(0, pikachu);   // Trainer's first Pokemon
    POKEMON_CAPTURE(1, charizard); // Trainer's second Pokemon
    
    // Set trainer money
    POKEMON_SET_TRAINER_MONEY(1, 5000);
    
    // Retrieve and display data
    TrainerData retrievedTrainer = POKEMON_GET_TRAINER(1);
    ESP_LOGI("DEMO", "Trainer: %s, Money: %d", retrievedTrainer.name, POKEMON_GET_TRAINER_MONEY(1));
    
    PokemonInstance retrievedPikachu = POKEMON_GET_CAPTURED(0);
    ESP_LOGI("DEMO", "Pokemon: %s, Level: %d", retrievedPikachu.nickname, (int)retrievedPikachu.level);
    
    // Display database stats
    ESP_LOGI("DEMO", "Pokemon DB - Total entries: %d, Used: %zu/%zu bytes", 
             (int)database.getEntryCount(PARTITION_SAVE),
             database.getTotalUsedBytes(),
             (POKEMON_CONFIG.romSize + POKEMON_CONFIG.saveSize + POKEMON_CONFIG.backupSize + POKEMON_CONFIG.runtimeSize));
    
    database.cleanup();
}

void demonstrateSnakeConfig() {
    ESP_LOGI("DEMO", "\n=== Snake Game Database Demo ===");
    
    // Initialize with Snake configuration
    ErrorCode result = database.initialize(&SNAKE_CONFIG);
    if (result != SUCCESS) {
        ESP_LOGE("DEMO", "Failed to initialize Snake database: %d", (int)result);
        return;
    }
    
    // Set up initial game state
    SNAKE_SET_HIGH_SCORE(1250);
    SNAKE_SET_GAME_STATE(3, 150, 5, true);  // Level 3, 150 score, 5 length, game active
    SNAKE_SAVE_SETTINGS(7, true, 2);        // Speed 7, sound on, difficulty 2
    
    // Add some snake segments
    SNAKE_SET_SEGMENT(0, 10, 10);  // Head
    SNAKE_SET_SEGMENT(1, 9, 10);   // Body
    SNAKE_SET_SEGMENT(2, 8, 10);   // Body
    SNAKE_SET_SEGMENT(3, 7, 10);   // Body
    SNAKE_SET_SEGMENT(4, 6, 10);   // Tail
    
    // Set food position
    SNAKE_SET_FOOD(15, 8);
    
    // Retrieve and display data
    uint16_t highScore = SNAKE_GET_HIGH_SCORE();
    GameState state = SNAKE_GET_GAME_STATE();
    GameSettings settings = SNAKE_LOAD_SETTINGS();
    
    ESP_LOGI("DEMO", "High Score: %u", highScore);
    ESP_LOGI("DEMO", "Current - Level: %d, Score: %u, Length: %d", (int)state.level, state.score, (int)state.snakeLength);
    ESP_LOGI("DEMO", "Settings - Speed: %d, Sound: %s, Difficulty: %d", 
             (int)settings.speed, (settings.soundEnabled ? "On" : "Off"), (int)settings.difficulty);
    
    SnakeSegment head = SNAKE_GET_SEGMENT(0);
    FoodPosition food = SNAKE_GET_FOOD();
    ESP_LOGI("DEMO", "Snake head at (%d,%d), Food at (%d,%d)", (int)head.x, (int)head.y, (int)food.x, (int)food.y);
    
    // Display database stats
    ESP_LOGI("DEMO", "Snake DB - Total entries: %d, Used: %zu/%zu bytes",
             (int)(database.getEntryCount(PARTITION_SAVE) + database.getEntryCount(PARTITION_RUNTIME)),
             database.getTotalUsedBytes(),
             (SNAKE_CONFIG.romSize + SNAKE_CONFIG.saveSize + SNAKE_CONFIG.backupSize + SNAKE_CONFIG.runtimeSize));
    
    database.cleanup();
}

void demonstrateIoTConfig() {
    ESP_LOGI("DEMO", "\n=== IoT Sensor Hub Database Demo ===");
    
    // Initialize with IoT configuration
    ErrorCode result = database.initialize(&IOT_CONFIG);
    if (result != SUCCESS) {
        ESP_LOGE("DEMO", "Failed to initialize IoT database: %d", (int)result);
        return;
    }
    
    // Define sensors
    IOT_DEFINE_SENSOR(1, SENSOR_TEMPERATURE, 34, 5000, "LivRoom");
    IOT_DEFINE_SENSOR(2, SENSOR_HUMIDITY, 35, 10000, "Bathroom");
    IOT_DEFINE_SENSOR(3, SENSOR_MOTION, 12, 1000, "FrontDr");
    
    // Log some sensor readings (scaled values)
    IOT_LOG_READING(1, 22.5f, 95);    // Temperature: 22.5°C, 95% quality
    IOT_LOG_READING(2, 65.0f, 90);    // Humidity: 65%, 90% quality
    IOT_LOG_READING(3, 1.0f, 100);    // Motion: detected, 100% quality
    
    // Set up some devices
    IOT_SET_DEVICE_STATE(1, 1, 255);  // LED on full brightness
    IOT_SET_DEVICE_STATE(2, 0, 0);    // Fan off
    
    // Configure WiFi
    IOT_SET_WIFI_CONFIG("MyHomeWiFi", "secretpass123");
    
    // Create automation rule: if temperature > 25°C, turn on fan
    IOT_CREATE_RULE(1, 1, 25.0f, 2, 1);  // Rule 1: sensor 1 > 25.0, activate device 2
    
    // Retrieve and display data
    SensorReading tempReading = IOT_GET_CURRENT_READING(1);
    SensorReading humidityReading = IOT_GET_CURRENT_READING(2);
    DeviceState ledState = IOT_GET_DEVICE_STATE(1);
    WiFiConfig wifi = IOT_GET_WIFI_CONFIG();
    
    ESP_LOGI("DEMO", "Temperature: %.1f°C (Quality: %d%%)", (tempReading.value / 100.0f), (int)tempReading.quality);
    ESP_LOGI("DEMO", "Humidity: %.1f%% (Quality: %d%%)", (humidityReading.value / 100.0f), (int)humidityReading.quality);
    ESP_LOGI("DEMO", "LED State: %s (Value: %d)", (ledState.state ? "ON" : "OFF"), ledState.value);
    ESP_LOGI("DEMO", "WiFi SSID: %s (DHCP: %s)", wifi.ssid, (wifi.dhcp ? "Yes" : "No"));
    
    // Display database stats
    ESP_LOGI("DEMO", "IoT DB - Total entries: %d, Used: %zu/%zu bytes",
             (int)(database.getEntryCount(PARTITION_SAVE) + database.getEntryCount(PARTITION_RUNTIME)),
             database.getTotalUsedBytes(),
             (IOT_CONFIG.romSize + IOT_CONFIG.saveSize + IOT_CONFIG.backupSize + IOT_CONFIG.runtimeSize));
    ESP_LOGI("DEMO", "Encryption: %s, Compression: %s",
             (IOT_CONFIG.enableEncryption ? "Enabled" : "Disabled"),
             (IOT_CONFIG.enableCompression ? "Enabled" : "Disabled"));
    
    database.cleanup();
}

void demonstrateMultiConfig() {
    ESP_LOGI("DEMO", "\n=== Multi-Configuration Comparison ===");
    
    // Compare different configurations
    ESP_LOGI("DEMO", "Configuration Comparison:");
    ESP_LOGI("DEMO", "App               | ROM  | Save | Backup | Runtime | Total | %% LP-SRAM");
    ESP_LOGI("DEMO", "------------------|------|------|--------|---------|-------|----------");
    
    struct ConfigData {
        const char* name;
        const PartitionConfig* config;
    } configs[] = {
        {"Pokemon RPG", &POKEMON_CONFIG},
        {"Snake Game", &SNAKE_CONFIG},
        {"IoT Sensors", &IOT_CONFIG}
    };
    
    for (int i = 0; i < 3; i++) {
        const PartitionConfig* cfg = configs[i].config;
        uint16_t total = cfg->romSize + cfg->saveSize + cfg->backupSize + cfg->runtimeSize;
        float percentage = (total * 100.0f) / LP_SRAM_SIZE;
        
        ESP_LOGI("DEMO", "%-17s | %4d | %4d | %6d | %7d | %5d | %6.1f%%",
                 configs[i].name, cfg->romSize, cfg->saveSize, cfg->backupSize, 
                 cfg->runtimeSize, total, percentage);
    }
    
    ESP_LOGI("DEMO", "\nFeature Comparison:");
    ESP_LOGI("DEMO", "Pokemon RPG: Cache=%d, Compression=%s, Encryption=%s",
             POKEMON_CONFIG.maxCacheEntries,
             (POKEMON_CONFIG.enableCompression ? "Yes" : "No"),
             (POKEMON_CONFIG.enableEncryption ? "Yes" : "No"));
    ESP_LOGI("DEMO", "Snake Game:  Cache=%d, Compression=%s, Encryption=%s",
             SNAKE_CONFIG.maxCacheEntries,
             (SNAKE_CONFIG.enableCompression ? "Yes" : "No"),
             (SNAKE_CONFIG.enableEncryption ? "Yes" : "No"));
    ESP_LOGI("DEMO", "IoT Sensors: Cache=%d, Compression=%s, Encryption=%s",
             IOT_CONFIG.maxCacheEntries,
             (IOT_CONFIG.enableCompression ? "Yes" : "No"),
             (IOT_CONFIG.enableEncryption ? "Yes" : "No"));
}

int main() {
    ESP_LOGI("DEMO", "Wisp Engine - Partitioned Database System Demo");
    ESP_LOGI("DEMO", "Restructured Architecture with Proper Organization");
    ESP_LOGI("DEMO", "=================================================");
    ESP_LOGI("DEMO", "LP-SRAM Size: %d bytes (16KB)", LP_SRAM_SIZE);
    ESP_LOGI("DEMO", "Max Entry Size: %d bytes", MAX_ENTRY_SIZE);
    ESP_LOGI("DEMO", "Entry Header Size: %d bytes", ENTRY_HEADER_SIZE);
    ESP_LOGI("DEMO", "Partition Header Size: %d bytes", PARTITION_HEADER_SIZE);
    
    // Demonstrate each configuration
    demonstratePokemonConfig();
    demonstrateSnakeConfig();
    demonstrateIoTConfig();
    demonstrateMultiConfig();
    
    ESP_LOGI("DEMO", "\n=== Demo Complete ===");
    ESP_LOGI("DEMO", "✅ Clean architecture: src/engine/database/partitioned_system.h");
    ESP_LOGI("DEMO", "✅ App-specific configs: examples/apps/{app_name}/database_config.h");
    ESP_LOGI("DEMO", "✅ No 'wisp_' prefixes - proper namespace organization");
    ESP_LOGI("DEMO", "✅ Memory-safe configurations from 2.25KB (14%%) to 13.75KB (86%%)");
    ESP_LOGI("DEMO", "✅ Comprehensive bounds checking and overflow protection");
    ESP_LOGI("DEMO", "✅ Efficient data structures optimized for 16KB LP-SRAM");
    
    return 0;
}
