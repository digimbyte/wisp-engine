# Wisp Engine Save System Developer Guide

## Overview

The Wisp Engine Save System provides a robust, app-fingerprinted, and null-safe way to persist game data. Each app must have a unique UUID to prevent save file conflicts, and the system supports modular save structures that adapt to each developer's needs.

## Key Features

- **App Fingerprinting**: Each save file is tied to a specific app UUID and version
- **Type Safety**: Strong typing with compile-time checks
- **Null Safety**: All operations are null-safe with comprehensive error handling
- **Auto-Save**: Optional automatic saving with configurable intervals
- **Corruption Protection**: CRC32 checksums and backup files
- **Memory Efficient**: Lazy loading and minimal overhead
- **Modular Structure**: Register only the data fields you need

## Basic Usage

### 1. Set App Identity

Every app MUST set a unique identity before using the save system:

```cpp
class MyGame : public WispAppBase {
    bool init() override {
        // CRITICAL: Use reverse domain notation for uniqueness
        if (!api->setAppIdentity("com.mystudio.mygame", "1.0.0", 1)) {
            return false; // Failed to set identity
        }
        
        // ... rest of initialization
    }
};
```

**UUID Format Guidelines:**
- Use reverse domain notation: `com.developer.gamename`
- Keep it unique and consistent across versions
- Examples: `com.johnsmith.platformer`, `org.indiedev.puzzlegame`

### 2. Register Save Fields

Register the variables you want to save during app initialization:

```cpp
class MyGame : public WispAppBase {
private:
    // Variables that will be saved
    int32_t playerLevel;
    int32_t currentScore;
    float musicVolume;
    bool tutorialCompleted;
    String playerName;
    uint8_t gameSettings[32];
    
    bool init() override {
        // Set identity first
        api->setAppIdentity("com.mystudio.mygame", "1.0.0");
        
        // Register save fields
        api->registerSaveField("level", &playerLevel);
        api->registerSaveField("score", &currentScore);
        api->registerSaveField("music_volume", &musicVolume);
        api->registerSaveField("tutorial_done", &tutorialCompleted);
        api->registerSaveField("player_name", &playerName, 64); // Max 64 chars
        api->registerSaveBlob("settings", gameSettings, sizeof(gameSettings));
        
        // Enable auto-save every 30 seconds
        api->enableAutoSave(true, 30000);
        
        // Try to load existing save
        if (api->hasSaveFile()) {
            api->load();
        }
        
        return true;
    }
};
```

### 3. Save and Load

```cpp
// Manual save (usually for important events)
if (playerWon) {
    api->save(); // Save immediately after winning
}

// Load existing save (usually in init())
if (api->hasSaveFile()) {
    if (api->load()) {
        // Load successful
    } else {
        // Load failed - corrupted file or version mismatch
    }
}

// Check if save file exists
if (api->hasSaveFile()) {
    // Show "Continue Game" option
} else {
    // Show "New Game" only
}
```

## Supported Data Types

### Basic Types
```cpp
bool gameCompleted;
int8_t difficulty;          // -128 to 127
uint8_t unlockedLevels;     // 0 to 255
int16_t playerHealth;       // -32768 to 32767
uint16_t itemCount;         // 0 to 65535
int32_t experience;         // Large signed numbers
uint32_t playTime;          // Large unsigned numbers (milliseconds)
float volume;               // Floating point values

// Register them
api->registerSaveField("completed", &gameCompleted);
api->registerSaveField("difficulty", &difficulty);
api->registerSaveField("levels", &unlockedLevels);
api->registerSaveField("health", &playerHealth);
api->registerSaveField("items", &itemCount);
api->registerSaveField("xp", &experience);
api->registerSaveField("playtime", &playTime);
api->registerSaveField("volume", &volume);
```

### Strings
```cpp
String playerName;
String lastMap;

// Register with maximum length
api->registerSaveField("name", &playerName, 32);    // Max 32 characters
api->registerSaveField("map", &lastMap, 128);       // Max 128 characters
```

### Binary Data (Blobs)
```cpp
struct GameSettings {
    uint8_t soundVolume;
    uint8_t musicVolume;
    bool enableParticles;
    uint32_t controllerConfig;
} settings;

uint8_t inventoryData[256];  // Raw inventory data
uint8_t mapProgress[64];     // Level completion flags

// Register blobs
api->registerSaveBlob("settings", &settings, sizeof(settings));
api->registerSaveBlob("inventory", inventoryData, sizeof(inventoryData));
api->registerSaveBlob("progress", mapProgress, sizeof(mapProgress));
```

## Advanced Features

### Accessing Save Data Safely

```cpp
// Get pointers to save data (null-safe)
int32_t* levelPtr = api->getSaveField<int32_t>("level");
if (levelPtr) {
    *levelPtr = newLevel; // Modify directly
}

String* namePtr = api->getSaveString("name");
if (namePtr) {
    *namePtr = "New Name"; // Modify string
}

// Or use the setter methods
api->setSaveField("level", newLevel);
api->setSaveString("name", "New Name");
```

### Auto-Save Configuration

```cpp
// Enable auto-save every 60 seconds
api->enableAutoSave(true, 60000);

// Disable auto-save (manual save only)
api->enableAutoSave(false);

// Check if save system is ready
if (api->isSaveSystemReady()) {
    // Safe to use save functions
}
```

### Save File Management

```cpp
// Check save file information
if (api->hasSaveFile()) {
    size_t fileSize = api->getSaveFileSize();
    uint64_t timestamp = api->getSaveTimestamp();
    
    Serial.println("Save file: " + String(fileSize) + " bytes");
    Serial.println("Last saved: " + String(timestamp) + "ms ago");
}

// Delete save file (for "New Game" option)
if (startNewGame) {
    api->deleteSaveFile();
    api->resetSaveData(); // Reset variables to defaults
}
```

## Best Practices

### 1. Choose Good Field Names
```cpp
// Good - descriptive and consistent
api->registerSaveField("player_level", &level);
api->registerSaveField("current_score", &score);
api->registerSaveField("music_volume", &volume);

// Bad - unclear or inconsistent
api->registerSaveField("l", &level);
api->registerSaveField("Score", &score);
api->registerSaveField("vol_music", &volume);
```

### 2. Handle Loading Gracefully
```cpp
bool loadGameData() {
    if (!api->hasSaveFile()) {
        // No save file - set defaults
        playerLevel = 1;
        currentScore = 0;
        return true; // This is OK for first run
    }
    
    if (api->load()) {
        // Validate loaded data
        if (playerLevel < 1) playerLevel = 1;
        if (currentScore < 0) currentScore = 0;
        return true;
    } else {
        // Load failed - offer recovery options
        api->printWarning("Save file corrupted - starting fresh");
        setDefaults();
        return false;
    }
}
```

### 3. Save on Important Events
```cpp
void onLevelComplete() {
    currentLevel++;
    totalScore += levelScore;
    
    // Save immediately after important progress
    api->save();
}

void onGameOver() {
    if (currentScore > highScore) {
        highScore = currentScore;
        api->save(); // Save new high score
    }
}

void onSettingsChanged() {
    // Settings should be saved immediately
    api->save();
}
```

### 4. Version Management
```cpp
// When updating your app with new save data:
api->setAppIdentity("com.mystudio.mygame", "1.1.0", 2); // New save format version

// The system will warn about version mismatches but still try to load
// You can check the version and migrate data if needed
```

## Error Handling

### Common Error Scenarios
```cpp
// 1. Save system not initialized
if (!api->isSaveSystemReady()) {
    api->printError("Save system not available");
    return;
}

// 2. Invalid field access
int32_t* score = api->getSaveField<int32_t>("score");
if (!score) {
    api->printError("Score field not found");
    return;
}

// 3. Save operation failed
if (!api->save()) {
    api->printError("Failed to save game - check storage space");
}

// 4. Load operation failed
if (!api->load()) {
    api->printWarning("Could not load save - starting fresh");
    setDefaults();
}
```

### Storage Issues
```cpp
// Check available storage before saving large amounts of data
size_t currentSize = api->getSaveFileSize();
if (currentSize > 10240) { // 10KB limit
    api->printWarning("Save file getting large");
}

// Handle storage full scenarios
if (!api->save()) {
    // Try cleaning up and saving again
    api->resetSaveData(); // Clear to defaults
    if (!api->save()) {
        api->printError("Storage may be full");
    }
}
```

## Example: Complete Save Integration

```cpp
class PlatformerGame : public WispAppBase {
private:
    // Save data
    int32_t currentLevel;
    int32_t totalCoins;
    int32_t highScore;
    bool[] levelUnlocked; // Array of 20 levels
    String playerName;
    
public:
    bool init() override {
        // 1. Set unique app identity
        if (!api->setAppIdentity("com.example.platformer", "1.0.0")) {
            return false;
        }
        
        // 2. Register save fields
        if (!registerSaveFields()) {
            return false;
        }
        
        // 3. Load existing save or set defaults
        if (!loadGameData()) {
            setDefaults();
        }
        
        // 4. Enable auto-save
        api->enableAutoSave(true, 45000); // Every 45 seconds
        
        return true;
    }
    
    void update() override {
        // Game logic that modifies save data
        if (coinCollected) {
            totalCoins++;
        }
        
        if (levelCompleted) {
            currentLevel++;
            levelUnlocked[currentLevel] = true;
            api->save(); // Save immediately on level completion
        }
    }
    
    void cleanup() override {
        // Final save before exit
        api->save();
    }
    
private:
    bool registerSaveFields() {
        return api->registerSaveField("level", &currentLevel) &&
               api->registerSaveField("coins", &totalCoins) &&
               api->registerSaveField("high_score", &highScore) &&
               api->registerSaveBlob("unlocked", levelUnlocked, sizeof(levelUnlocked)) &&
               api->registerSaveField("name", &playerName, 32);
    }
    
    bool loadGameData() {
        if (api->hasSaveFile()) {
            return api->load();
        }
        return true; // No save file is OK
    }
    
    void setDefaults() {
        currentLevel = 1;
        totalCoins = 0;
        highScore = 0;
        memset(levelUnlocked, 0, sizeof(levelUnlocked));
        levelUnlocked[0] = true; // First level unlocked
        playerName = "Player";
    }
};
```

## File Format Details

The save system uses a binary format with these characteristics:

- **Header**: Contains app UUID, version, checksum, and metadata
- **Data Section**: Serialized field data with type information
- **Backup**: Automatic backup files created before each save
- **Validation**: CRC32 checksums prevent corruption
- **Compatibility**: Version checking with graceful degradation

## Troubleshooting

### Save File Not Loading
1. Check app UUID is consistent
2. Verify all registered fields exist
3. Check storage permissions
4. Look for corruption (checksum mismatch)

### Performance Issues
1. Reduce auto-save frequency
2. Minimize blob data size
3. Register only necessary fields
4. Use appropriate data types

### Storage Full
1. Implement save file cleanup
2. Provide user warning
3. Offer data export/import
4. Use smaller data types where possible

---

This save system provides a robust foundation for game data persistence while maintaining security and preventing conflicts between different apps. The app fingerprinting ensures that each game's save data remains isolated and secure.
