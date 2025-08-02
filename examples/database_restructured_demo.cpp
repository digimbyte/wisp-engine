#include "../engine/database/partitioned_system.h"

// Include different app configurations
#include "apps/pokemon_rpg/database_config.h"
#include "apps/snake_game/database_config.h"
#include "apps/iot_sensor_hub/database_config.h"

#include <iostream>
#include <iomanip>

void demonstratePokemonConfig() {
    std::cout << "\n=== Pokemon RPG Database Demo ===\n";
    
    // Initialize with Pokemon configuration
    ErrorCode result = database.initialize(&POKEMON_CONFIG);
    if (result != SUCCESS) {
        std::cout << "Failed to initialize Pokemon database: " << (int)result << std::endl;
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
    std::cout << "Trainer: " << retrievedTrainer.name << ", Money: " << POKEMON_GET_TRAINER_MONEY(1) << std::endl;
    
    PokemonInstance retrievedPikachu = POKEMON_GET_CAPTURED(0);
    std::cout << "Pokemon: " << retrievedPikachu.nickname << ", Level: " << (int)retrievedPikachu.level << std::endl;
    
    // Display database stats
    std::cout << "Pokemon DB - Total entries: " << (int)database.getEntryCount(PARTITION_SAVE);
    std::cout << ", Used: " << database.getTotalUsedBytes() << "/" << 
                 (POKEMON_CONFIG.romSize + POKEMON_CONFIG.saveSize + POKEMON_CONFIG.backupSize + POKEMON_CONFIG.runtimeSize) << " bytes" << std::endl;
    
    database.cleanup();
}

void demonstrateSnakeConfig() {
    std::cout << "\n=== Snake Game Database Demo ===\n";
    
    // Initialize with Snake configuration
    ErrorCode result = database.initialize(&SNAKE_CONFIG);
    if (result != SUCCESS) {
        std::cout << "Failed to initialize Snake database: " << (int)result << std::endl;
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
    
    std::cout << "High Score: " << highScore << std::endl;
    std::cout << "Current - Level: " << (int)state.level << ", Score: " << state.score;
    std::cout << ", Length: " << (int)state.snakeLength << std::endl;
    std::cout << "Settings - Speed: " << (int)settings.speed << ", Sound: " << (settings.soundEnabled ? "On" : "Off");
    std::cout << ", Difficulty: " << (int)settings.difficulty << std::endl;
    
    SnakeSegment head = SNAKE_GET_SEGMENT(0);
    FoodPosition food = SNAKE_GET_FOOD();
    std::cout << "Snake head at (" << (int)head.x << "," << (int)head.y << ")";
    std::cout << ", Food at (" << (int)food.x << "," << (int)food.y << ")" << std::endl;
    
    // Display database stats
    std::cout << "Snake DB - Total entries: " << (int)(database.getEntryCount(PARTITION_SAVE) + database.getEntryCount(PARTITION_RUNTIME));
    std::cout << ", Used: " << database.getTotalUsedBytes() << "/" << 
                 (SNAKE_CONFIG.romSize + SNAKE_CONFIG.saveSize + SNAKE_CONFIG.backupSize + SNAKE_CONFIG.runtimeSize) << " bytes" << std::endl;
    
    database.cleanup();
}

void demonstrateIoTConfig() {
    std::cout << "\n=== IoT Sensor Hub Database Demo ===\n";
    
    // Initialize with IoT configuration
    ErrorCode result = database.initialize(&IOT_CONFIG);
    if (result != SUCCESS) {
        std::cout << "Failed to initialize IoT database: " << (int)result << std::endl;
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
    
    std::cout << "Temperature: " << (tempReading.value / 100.0f) << "°C (Quality: " << (int)tempReading.quality << "%)" << std::endl;
    std::cout << "Humidity: " << (humidityReading.value / 100.0f) << "% (Quality: " << (int)humidityReading.quality << "%)" << std::endl;
    std::cout << "LED State: " << (ledState.state ? "ON" : "OFF") << " (Value: " << ledState.value << ")" << std::endl;
    std::cout << "WiFi SSID: " << wifi.ssid << " (DHCP: " << (wifi.dhcp ? "Yes" : "No") << ")" << std::endl;
    
    // Display database stats
    std::cout << "IoT DB - Total entries: " << (int)(database.getEntryCount(PARTITION_SAVE) + database.getEntryCount(PARTITION_RUNTIME));
    std::cout << ", Used: " << database.getTotalUsedBytes() << "/" << 
                 (IOT_CONFIG.romSize + IOT_CONFIG.saveSize + IOT_CONFIG.backupSize + IOT_CONFIG.runtimeSize) << " bytes" << std::endl;
    std::cout << "Encryption: " << (IOT_CONFIG.enableEncryption ? "Enabled" : "Disabled");
    std::cout << ", Compression: " << (IOT_CONFIG.enableCompression ? "Enabled" : "Disabled") << std::endl;
    
    database.cleanup();
}

void demonstrateMultiConfig() {
    std::cout << "\n=== Multi-Configuration Comparison ===\n";
    
    // Compare different configurations
    std::cout << "Configuration Comparison:\n";
    std::cout << "App               | ROM  | Save | Backup | Runtime | Total | % LP-SRAM\n";
    std::cout << "------------------|------|------|--------|---------|-------|----------\n";
    
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
        
        std::cout << std::left << std::setw(18) << configs[i].name << "| ";
        std::cout << std::right << std::setw(4) << cfg->romSize << " | ";
        std::cout << std::setw(4) << cfg->saveSize << " | ";
        std::cout << std::setw(6) << cfg->backupSize << " | ";
        std::cout << std::setw(7) << cfg->runtimeSize << " | ";
        std::cout << std::setw(5) << total << " | ";
        std::cout << std::setw(6) << std::fixed << std::setprecision(1) << percentage << "%\n";
    }
    
    std::cout << "\nFeature Comparison:\n";
    std::cout << "Pokemon RPG: Cache=" << POKEMON_CONFIG.maxCacheEntries << ", Compression=" << 
                 (POKEMON_CONFIG.enableCompression ? "Yes" : "No") << ", Encryption=" << 
                 (POKEMON_CONFIG.enableEncryption ? "Yes" : "No") << std::endl;
    std::cout << "Snake Game:  Cache=" << SNAKE_CONFIG.maxCacheEntries << ", Compression=" << 
                 (SNAKE_CONFIG.enableCompression ? "Yes" : "No") << ", Encryption=" << 
                 (SNAKE_CONFIG.enableEncryption ? "Yes" : "No") << std::endl;
    std::cout << "IoT Sensors: Cache=" << IOT_CONFIG.maxCacheEntries << ", Compression=" << 
                 (IOT_CONFIG.enableCompression ? "Yes" : "No") << ", Encryption=" << 
                 (IOT_CONFIG.enableEncryption ? "Yes" : "No") << std::endl;
}

int main() {
    std::cout << "Wisp Engine - Partitioned Database System Demo\n";
    std::cout << "Restructured Architecture with Proper Organization\n";
    std::cout << "=================================================\n";
    std::cout << "LP-SRAM Size: " << LP_SRAM_SIZE << " bytes (16KB)\n";
    std::cout << "Max Entry Size: " << MAX_ENTRY_SIZE << " bytes\n";
    std::cout << "Entry Header Size: " << ENTRY_HEADER_SIZE << " bytes\n";
    std::cout << "Partition Header Size: " << PARTITION_HEADER_SIZE << " bytes\n";
    
    // Demonstrate each configuration
    demonstratePokemonConfig();
    demonstrateSnakeConfig();
    demonstrateIoTConfig();
    demonstrateMultiConfig();
    
    std::cout << "\n=== Demo Complete ===\n";
    std::cout << "✅ Clean architecture: src/engine/database/partitioned_system.h\n";
    std::cout << "✅ App-specific configs: examples/apps/{app_name}/database_config.h\n";
    std::cout << "✅ No 'wisp_' prefixes - proper namespace organization\n";
    std::cout << "✅ Memory-safe configurations from 2.25KB (14%) to 13.75KB (86%)\n";
    std::cout << "✅ Comprehensive bounds checking and overflow protection\n";
    std::cout << "✅ Efficient data structures optimized for 16KB LP-SRAM\n";
    
    return 0;
}
