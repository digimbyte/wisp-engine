#include "../src/engine/database/database_system.h"

// Include different app configurations
#include "pokemon_rpg_db_config.h"
#include "snake_game_db_config.h"
#include "iot_sensor_db_config.h"

#include "esp_log.h"

void demonstratePokemonConfig() {
    ESP_LOGI("DEMO", "\n=== Pokemon RPG Database Demo ===");
    
    // Initialize with Pokemon configuration
    WispErrorCode result = POKEMON_DB_INIT();
    if (result != WISP_SUCCESS) {
        ESP_LOGE("DEMO", "Failed to initialize Pokemon database: %d", result);
        return;
    }
    
    // Add some Pokemon data
    POKEMON_DEFINE_SPECIES(1, "Pikachu", TYPE_ELECTRIC, 25, 112, 55, 40, 50, 90);
    POKEMON_DEFINE_SPECIES(6, "Charizard", TYPE_FIRE, 78, 266, 84, 78, 85, 100);
    
    // Create a trainer
    TrainerData trainer = {1, "Ash", 0, 0, 1000, {1, 6, 0, 0, 0, 0}};
    wispDB.set(TRAINER_KEY(1), &trainer, sizeof(trainer), ENTRY_TRAINER_DATA);
    
    // Add some captured Pokemon
    PokemonInstance pikachu = {1, 1, 25, {112, 55, 40, 50, 90}, 1000, "Pikachu"};
    PokemonInstance charizard = {2, 6, 55, {266, 84, 78, 85, 100}, 15000, "Charizard"};
    
    POKEMON_CAPTURE(1, pikachu);  // Trainer 1 captures Pikachu
    POKEMON_CAPTURE(2, charizard); // Trainer 1 captures Charizard
    
    // Retrieve and display data
    TrainerData retrievedTrainer = POKEMON_GET_TRAINER(1);
    ESP_LOGI("DEMO", "Trainer: %s, Money: %d", retrievedTrainer.name, retrievedTrainer.money);
    
    PokemonInstance retrievedPikachu = POKEMON_GET_CAPTURED(1);
    ESP_LOGI("DEMO", "Pokemon: %s, Level: %d", retrievedPikachu.nickname, (int)retrievedPikachu.level);
    
    // Display database stats
    WispDBStats stats;
    wispDB.getStats(&stats);
    ESP_LOGI("DEMO", "Pokemon DB - Total entries: %zu, Used: %zu/%zu bytes", stats.totalEntries, stats.usedSize, stats.totalSize);
    
    wispDB.cleanup();
}

void demonstrateSnakeConfig() {
    ESP_LOGI("DEMO", "\n=== Snake Game Database Demo ===");
    
    // Initialize with Snake configuration
    WispErrorCode result = SNAKE_DB_INIT();
    if (result != WISP_SUCCESS) {
        ESP_LOGE("DEMO", "Failed to initialize Snake database: %d", result);
        return;
    }
    
    // Set up initial game state
    SNAKE_SET_HIGH_SCORE(1250);
    SNAKE_SET_GAME_STATE(3, 150, 5, true);  // Level 3, 150 score, 5 length, game active
    
    // Add some snake segments
    SnakeSegment segments[] = {
        {10, 10}, {9, 10}, {8, 10}, {7, 10}, {6, 10}
    };
    
    for (int i = 0; i < 5; i++) {
        SNAKE_SET_SEGMENT(i, segments[i].x, segments[i].y);
    }
    
    // Set food position
    SNAKE_SET_FOOD(15, 8);
    
    // Retrieve and display data
    uint16_t highScore = SNAKE_GET_HIGH_SCORE();
    GameState state = SNAKE_GET_GAME_STATE();
    
    ESP_LOGI("DEMO", "High Score: %u", highScore);
    ESP_LOGI("DEMO", "Current - Level: %d, Score: %u, Length: %d", (int)state.level, state.score, (int)state.snakeLength);
    
    SnakeSegment head = SNAKE_GET_SEGMENT(0);
    FoodPosition food = SNAKE_GET_FOOD();
    ESP_LOGI("DEMO", "Snake head at (%d,%d), Food at (%d,%d)", (int)head.x, (int)head.y, (int)food.x, (int)food.y);
    
    // Display database stats
    WispDBStats stats;
    wispDB.getStats(&stats);
    ESP_LOGI("DEMO", "Snake DB - Total entries: %zu, Used: %zu/%zu bytes", stats.totalEntries, stats.usedSize, stats.totalSize);
    
    wispDB.cleanup();
}

void demonstrateIoTConfig() {
    ESP_LOGI("DEMO", "\n=== IoT Sensor Hub Database Demo ===");
    
    // Initialize with IoT configuration  
    WispErrorCode result = IOT_DB_INIT();
    if (result != WISP_SUCCESS) {
        ESP_LOGE("DEMO", "Failed to initialize IoT database: %d", result);
        return;
    }    // Define sensors
    IOT_DEFINE_SENSOR(1, SENSOR_TEMPERATURE, "Living Room", 34, 5000);
    IOT_DEFINE_SENSOR(2, SENSOR_HUMIDITY, "Bathroom", 35, 10000);
    IOT_DEFINE_SENSOR(3, SENSOR_MOTION, "Front Door", 12, 1000);
    
    // Log some sensor readings
    IOT_LOG_READING(1, 22.5f, 95);    // Temperature: 22.5°C, 95% quality
    IOT_LOG_READING(2, 65.0f, 90);    // Humidity: 65%, 90% quality
    IOT_LOG_READING(3, 1.0f, 100);    // Motion: detected, 100% quality
    
    // Set up some devices
    IOT_SET_DEVICE_STATE(1, 1, 255);  // LED on full brightness
    IOT_SET_DEVICE_STATE(2, 0, 0);    // Fan off
    
    // Configure WiFi
    IOT_SET_WIFI_CONFIG("MyHomeWiFi", "secretpassword123");
    
    // Create automation rule: if temperature > 25°C, turn on fan
    IOT_CREATE_RULE(1, 1, 25.0f, 2, 1);  // Rule 1: sensor 1 > 25.0, activate device 2
    
    // Retrieve and display data
    SensorReading tempReading = IOT_GET_CURRENT_READING(1);
    SensorReading humidityReading = IOT_GET_CURRENT_READING(2);
    DeviceState ledState = IOT_GET_DEVICE_STATE(1);
    WiFiConfig wifi = IOT_GET_WIFI_CONFIG();
    
    ESP_LOGI("DEMO", "Temperature: %.1f°C (Quality: %d%%)", tempReading.value, (int)tempReading.quality);
    ESP_LOGI("DEMO", "Humidity: %.1f%% (Quality: %d%%)", humidityReading.value, (int)humidityReading.quality);
    ESP_LOGI("DEMO", "LED State: %s (Value: %d)", (ledState.state ? "ON" : "OFF"), ledState.value);
    ESP_LOGI("DEMO", "WiFi SSID: %s (DHCP: %s)", wifi.ssid, (wifi.dhcp ? "Yes" : "No"));
    
    // Display database stats
    WispDBStats stats;
    wispDB.getStats(&stats);
    ESP_LOGI("DEMO", "IoT DB - Total entries: %zu, Used: %zu/%zu bytes", stats.totalEntries, stats.usedSize, stats.totalSize);
    ESP_LOGI("DEMO", "Encryption: %s, Compression: %s", 
             (stats.encryptionEnabled ? "Enabled" : "Disabled"),
             (stats.compressionEnabled ? "Enabled" : "Disabled"));
    
    wispDB.cleanup();
}

void demonstrateMultiConfig() {
    ESP_LOGI("DEMO", "\n=== Multi-Configuration Comparison ===");
    
    // Compare different configurations
    ESP_LOGI("DEMO", "Configuration Comparison:");
    ESP_LOGI("DEMO", "Pokemon RPG: ROM=%zuB, Save=%zuB, Total=%zuB", 
                 POKEMON_DB_CONFIG.romSize, POKEMON_DB_CONFIG.saveSize,
                 (POKEMON_DB_CONFIG.romSize + POKEMON_DB_CONFIG.saveSize + POKEMON_DB_CONFIG.backupSize + POKEMON_DB_CONFIG.runtimeSize));
    ESP_LOGI("DEMO", "Snake Game:  ROM=%zuB, Save=%zuB, Total=%zuB", 
                 SNAKE_DB_CONFIG.romSize, SNAKE_DB_CONFIG.saveSize,
                 (SNAKE_DB_CONFIG.romSize + SNAKE_DB_CONFIG.saveSize + SNAKE_DB_CONFIG.backupSize + SNAKE_DB_CONFIG.runtimeSize));
    ESP_LOGI("DEMO", "IoT Sensors: ROM=%zuB, Save=%zuB, Total=%zuB", 
                 IOT_DB_CONFIG.romSize, IOT_DB_CONFIG.saveSize,
                 (IOT_DB_CONFIG.romSize + IOT_DB_CONFIG.saveSize + IOT_DB_CONFIG.backupSize + IOT_DB_CONFIG.runtimeSize));
    
    ESP_LOGI("DEMO", "\nFeature Comparison:");
    ESP_LOGI("DEMO", "Pokemon RPG: Cache=%d, Compression=%s, Encryption=%s", 
                 POKEMON_DB_CONFIG.maxCacheEntries,
                 (POKEMON_DB_CONFIG.enableCompression ? "Yes" : "No"),
                 (POKEMON_DB_CONFIG.enableEncryption ? "Yes" : "No"));
    ESP_LOGI("DEMO", "Snake Game:  Cache=%d, Compression=%s, Encryption=%s", 
                 SNAKE_DB_CONFIG.maxCacheEntries,
                 (SNAKE_DB_CONFIG.enableCompression ? "Yes" : "No"),
                 (SNAKE_DB_CONFIG.enableEncryption ? "Yes" : "No"));
    ESP_LOGI("DEMO", "IoT Sensors: Cache=%d, Compression=%s, Encryption=%s", 
                 IOT_DB_CONFIG.maxCacheEntries,
                 (IOT_DB_CONFIG.enableCompression ? "Yes" : "No"), 
                 (IOT_DB_CONFIG.enableEncryption ? "Yes" : "No"));
}

int main() {
    ESP_LOGI("DEMO", "Wisp Partitioned Database System V2 - Configuration Demo");
    ESP_LOGI("DEMO", "========================================================");
    
    // Demonstrate each configuration
    demonstratePokemonConfig();
    demonstrateSnakeConfig();
    demonstrateIoTConfig();
    demonstrateMultiConfig();
    
    ESP_LOGI("DEMO", "\n=== Demo Complete ===");
    ESP_LOGI("DEMO", "Each app can now define its own database partition sizes and features!");
    ESP_LOGI("DEMO", "The engine supports configurations from 3KB (Snake) to 16KB+ (Pokemon)");
    ESP_LOGI("DEMO", "with optional compression, encryption, and caching per app.");
    
    return 0;
}
