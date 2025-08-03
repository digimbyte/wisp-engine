#include "../src/engine/database/database_system.h"

// Include different app configurations
#include "pokemon_rpg_db_config.h"
#include "snake_game_db_config.h"
#include "iot_sensor_db_config.h"

#include <iostream>
#include <cstring>

void demonstratePokemonConfig() {
    std::cout << "\n=== Pokemon RPG Database Demo ===\n";
    
    // Initialize with Pokemon configuration
    WispErrorCode result = POKEMON_DB_INIT();
    if (result != WISP_SUCCESS) {
        std::cout << "Failed to initialize Pokemon database: " << result << std::endl;
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
    std::cout << "Trainer: " << retrievedTrainer.name << ", Money: " << retrievedTrainer.money << std::endl;
    
    PokemonInstance retrievedPikachu = POKEMON_GET_CAPTURED(1);
    std::cout << "Pokemon: " << retrievedPikachu.nickname << ", Level: " << (int)retrievedPikachu.level << std::endl;
    
    // Display database stats
    WispDBStats stats;
    wispDB.getStats(&stats);
    std::cout << "Pokemon DB - Total entries: " << stats.totalEntries;
    std::cout << ", Used: " << stats.usedSize << "/" << stats.totalSize << " bytes" << std::endl;
    
    wispDB.cleanup();
}

void demonstrateSnakeConfig() {
    std::cout << "\n=== Snake Game Database Demo ===\n";
    
    // Initialize with Snake configuration
    WispErrorCode result = SNAKE_DB_INIT();
    if (result != WISP_SUCCESS) {
        std::cout << "Failed to initialize Snake database: " << result << std::endl;
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
    
    std::cout << "High Score: " << highScore << std::endl;
    std::cout << "Current - Level: " << (int)state.level << ", Score: " << state.score;
    std::cout << ", Length: " << (int)state.snakeLength << std::endl;
    
    SnakeSegment head = SNAKE_GET_SEGMENT(0);
    FoodPosition food = SNAKE_GET_FOOD();
    std::cout << "Snake head at (" << (int)head.x << "," << (int)head.y << ")";
    std::cout << ", Food at (" << (int)food.x << "," << (int)food.y << ")" << std::endl;
    
    // Display database stats
    WispDBStats stats;
    wispDB.getStats(&stats);
    std::cout << "Snake DB - Total entries: " << stats.totalEntries;
    std::cout << ", Used: " << stats.usedSize << "/" << stats.totalSize << " bytes" << std::endl;
    
    wispDB.cleanup();
}

void demonstrateIoTConfig() {
    std::cout << "\n=== IoT Sensor Hub Database Demo ===\n";
    
    // Initialize with IoT configuration
    WispErrorCode result = IOT_DB_INIT();
    if (result != WISP_SUCCESS) {
        std::cout << "Failed to initialize IoT database: " << result << std::endl;
        return;
    }
    
    // Define sensors
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
    
    std::cout << "Temperature: " << tempReading.value << "°C (Quality: " << (int)tempReading.quality << "%)" << std::endl;
    std::cout << "Humidity: " << humidityReading.value << "% (Quality: " << (int)humidityReading.quality << "%)" << std::endl;
    std::cout << "LED State: " << (ledState.state ? "ON" : "OFF") << " (Value: " << ledState.value << ")" << std::endl;
    std::cout << "WiFi SSID: " << wifi.ssid << " (DHCP: " << (wifi.dhcp ? "Yes" : "No") << ")" << std::endl;
    
    // Display database stats
    WispDBStats stats;
    wispDB.getStats(&stats);
    std::cout << "IoT DB - Total entries: " << stats.totalEntries;
    std::cout << ", Used: " << stats.usedSize << "/" << stats.totalSize << " bytes" << std::endl;
    std::cout << "Encryption: " << (stats.encryptionEnabled ? "Enabled" : "Disabled");
    std::cout << ", Compression: " << (stats.compressionEnabled ? "Enabled" : "Disabled") << std::endl;
    
    wispDB.cleanup();
}

void demonstrateMultiConfig() {
    std::cout << "\n=== Multi-Configuration Comparison ===\n";
    
    // Compare different configurations
    std::cout << "Configuration Comparison:" << std::endl;
    std::cout << "Pokemon RPG: ROM=" << POKEMON_DB_CONFIG.romSize << "B, Save=" << POKEMON_DB_CONFIG.saveSize << "B, Total=" << 
                 (POKEMON_DB_CONFIG.romSize + POKEMON_DB_CONFIG.saveSize + POKEMON_DB_CONFIG.backupSize + POKEMON_DB_CONFIG.runtimeSize) << "B" << std::endl;
    std::cout << "Snake Game:  ROM=" << SNAKE_DB_CONFIG.romSize << "B, Save=" << SNAKE_DB_CONFIG.saveSize << "B, Total=" << 
                 (SNAKE_DB_CONFIG.romSize + SNAKE_DB_CONFIG.saveSize + SNAKE_DB_CONFIG.backupSize + SNAKE_DB_CONFIG.runtimeSize) << "B" << std::endl;
    std::cout << "IoT Sensors: ROM=" << IOT_DB_CONFIG.romSize << "B, Save=" << IOT_DB_CONFIG.saveSize << "B, Total=" << 
                 (IOT_DB_CONFIG.romSize + IOT_DB_CONFIG.saveSize + IOT_DB_CONFIG.backupSize + IOT_DB_CONFIG.runtimeSize) << "B" << std::endl;
    
    std::cout << "\nFeature Comparison:" << std::endl;
    std::cout << "Pokemon RPG: Cache=" << POKEMON_DB_CONFIG.maxCacheEntries << ", Compression=" << 
                 (POKEMON_DB_CONFIG.enableCompression ? "Yes" : "No") << ", Encryption=" << 
                 (POKEMON_DB_CONFIG.enableEncryption ? "Yes" : "No") << std::endl;
    std::cout << "Snake Game:  Cache=" << SNAKE_DB_CONFIG.maxCacheEntries << ", Compression=" << 
                 (SNAKE_DB_CONFIG.enableCompression ? "Yes" : "No") << ", Encryption=" << 
                 (SNAKE_DB_CONFIG.enableEncryption ? "Yes" : "No") << std::endl;
    std::cout << "IoT Sensors: Cache=" << IOT_DB_CONFIG.maxCacheEntries << ", Compression=" << 
                 (IOT_DB_CONFIG.enableCompression ? "Yes" : "No") << ", Encryption=" << 
                 (IOT_DB_CONFIG.enableEncryption ? "Yes" : "No") << std::endl;
}

int main() {
    std::cout << "Wisp Partitioned Database System V2 - Configuration Demo\n";
    std::cout << "========================================================\n";
    
    // Demonstrate each configuration
    demonstratePokemonConfig();
    demonstrateSnakeConfig();
    demonstrateIoTConfig();
    demonstrateMultiConfig();
    
    std::cout << "\n=== Demo Complete ===\n";
    std::cout << "Each app can now define its own database partition sizes and features!\n";
    std::cout << "The engine supports configurations from 3KB (Snake) to 16KB+ (Pokemon)\n";
    std::cout << "with optional compression, encryption, and caching per app.\n";
    
    return 0;
}
