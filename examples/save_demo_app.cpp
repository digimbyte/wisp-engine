// examples/save_demo_app.cpp
// Demonstration app showing how to use the Wisp Save System
#include "../src/engine/app/interface.h"
#include "../src/engine/app/curated_api.h"

class SaveDemoApp : public WispAppBase {
private:
    // Save data - these variables will be automatically saved/loaded
    int32_t playerLevel;
    int32_t playerExperience;
    int32_t highScore;
    float gameVolume;
    bool tutorialCompleted;
    String playerName;
    uint8_t settingsData[64]; // Example blob data
    
    // Game state (not saved)
    bool initialized;
    uint32_t lastLevelUpTime;
    uint32_t gameStartTime;
    
public:
    SaveDemoApp() : 
        playerLevel(1), playerExperience(0), highScore(0), 
        gameVolume(0.8f), tutorialCompleted(false), playerName("Player"),
        initialized(false), lastLevelUpTime(0), gameStartTime(0) {
        
        // Initialize blob data
        memset(settingsData, 0, sizeof(settingsData));
        settingsData[0] = 0xFF; // Magic marker
    }
    
    bool init() override {
        if (!api) {
            Serial.println("SaveDemo: API not available");
            return false;
        }
        
        // Set app identity with UUID (CRITICAL - must be unique per app)
        if (!api->setAppIdentity("com.wispengine.savedemo", "1.0.0", 1)) {
            Serial.println("SaveDemo: Failed to set app identity");
            return false;
        }
        
        // Register save fields - this links our variables to the save system
        // The system will automatically save/load these variables
        if (!registerSaveFields()) {
            Serial.println("SaveDemo: Failed to register save fields");
            return false;
        }
        
        // Enable auto-save every 30 seconds
        api->enableAutoSave(true, 30000);
        
        // Try to load existing save data
        if (api->hasSaveFile()) {
            Serial.println("SaveDemo: Loading existing save file...");
            if (api->load()) {
                Serial.println("SaveDemo: Save file loaded successfully");
                Serial.print("  Player Level: ");
                Serial.println(playerLevel);
                Serial.print("  Experience: ");
                Serial.println(playerExperience);
                Serial.print("  High Score: ");
                Serial.println(highScore);
                Serial.print("  Player Name: ");
                Serial.println(playerName);
                Serial.print("  Tutorial Completed: ");
                Serial.println(tutorialCompleted ? "Yes" : "No");
            } else {
                Serial.println("SaveDemo: Failed to load save file - starting fresh");
            }
        } else {
            Serial.println("SaveDemo: No save file found - starting fresh");
        }
        
        gameStartTime = api->getTime();
        initialized = true;
        
        api->print("Save Demo App initialized successfully");
        return true;
    }
    
    void update() override {
        if (!initialized) return;
        
        const WispInputState& input = api->getInput();
        uint32_t currentTime = api->getTime();
        
        // Simulate gaining experience over time
        if (currentTime - gameStartTime > 1000) { // Every second
            playerExperience += 10;
            gameStartTime = currentTime;
            
            // Check for level up
            int32_t xpNeeded = playerLevel * 100;
            if (playerExperience >= xpNeeded) {
                playerLevel++;
                playerExperience -= xpNeeded;
                lastLevelUpTime = currentTime;
                
                api->print("Level up! Now level " + String(playerLevel));
                
                // Manually save on important events
                api->save();
            }
        }
        
        // Button controls for testing save system
        static bool buttonPressed = false;
        
        if (input.buttonA && !buttonPressed) {
            // Button A: Increase high score
            highScore += 100;
            api->print("High score increased to " + String(highScore));
            buttonPressed = true;
            
        } else if (input.buttonB && !buttonPressed) {
            // Button B: Toggle tutorial completed
            tutorialCompleted = !tutorialCompleted;
            api->print("Tutorial completed: " + String(tutorialCompleted ? "Yes" : "No"));
            buttonPressed = true;
            
        } else if (input.up && !buttonPressed) {
            // Up: Increase volume
            gameVolume = min(1.0f, gameVolume + 0.1f);
            api->print("Volume: " + String(gameVolume * 100, 0) + "%");
            buttonPressed = true;
            
        } else if (input.down && !buttonPressed) {
            // Down: Decrease volume
            gameVolume = max(0.0f, gameVolume - 0.1f);
            api->print("Volume: " + String(gameVolume * 100, 0) + "%");
            buttonPressed = true;
            
        } else if (input.left && !buttonPressed) {
            // Left: Change player name
            changePlayerName();
            buttonPressed = true;
            
        } else if (input.right && !buttonPressed) {
            // Right: Reset save data
            resetProgress();
            buttonPressed = true;
            
        } else if (input.select && !buttonPressed) {
            // Select: Manual save
            if (api->save()) {
                api->print("Game saved manually");
            } else {
                api->printError("Failed to save game");
            }
            buttonPressed = true;
        }
        
        // Reset button state
        if (!input.buttonA && !input.buttonB && !input.up && !input.down && 
            !input.left && !input.right && !input.select) {
            buttonPressed = false;
        }
    }
    
    void render() override {
        if (!initialized) return;
        
        uint32_t currentTime = api->getTime();
        
        // Draw background
        api->drawRect(0, 0, 320, 240, WispColor(20, 20, 40), 0);
        
        // Draw title
        api->drawText("SAVE SYSTEM DEMO", 160, 20, WispColor(255, 255, 255), 10);
        
        // Draw save status
        String saveStatus = api->hasSaveFile() ? "Save file exists" : "No save file";
        api->drawText(saveStatus, 160, 45, WispColor(200, 200, 200), 9);
        
        // Draw player stats
        api->drawText("Player: " + playerName, 20, 70, WispColor(255, 255, 0), 8);
        api->drawText("Level: " + String(playerLevel), 20, 90, WispColor(255, 255, 0), 8);
        api->drawText("Experience: " + String(playerExperience) + "/" + String(playerLevel * 100), 20, 110, WispColor(255, 255, 0), 8);
        api->drawText("High Score: " + String(highScore), 20, 130, WispColor(255, 255, 0), 8);
        api->drawText("Volume: " + String(gameVolume * 100, 0) + "%", 20, 150, WispColor(255, 255, 0), 8);
        api->drawText("Tutorial: " + String(tutorialCompleted ? "Complete" : "Incomplete"), 20, 170, WispColor(255, 255, 0), 8);
        
        // Draw level up indicator
        if (currentTime - lastLevelUpTime < 3000) {
            api->drawText("LEVEL UP!", 160, 100, WispColor(255, 100, 100), 8);
        }
        
        // Draw controls
        api->drawText("Controls:", 20, 200, WispColor(150, 150, 150), 6);
        api->drawText("A: +Score  B: Tutorial  U/D: Volume", 20, 215, WispColor(100, 100, 100), 6);
        api->drawText("L: Name  R: Reset  SELECT: Save", 20, 225, WispColor(100, 100, 100), 6);
        
        // Draw save file info
        if (api->hasSaveFile()) {
            String fileInfo = "Save: " + String(api->getSaveFileSize()) + " bytes";
            api->drawText(fileInfo, 200, 200, WispColor(100, 200, 100), 6);
        }
    }
    
    void cleanup() override {
        if (initialized) {
            // Force save before cleanup
            if (api->save()) {
                api->print("Final save completed");
            } else {
                api->printError("Final save failed");
            }
        }
        
        api->print("Save Demo App cleaned up");
    }
    
private:
    bool registerSaveFields() {
        // Register all the variables we want to save
        // The save system will automatically handle serialization
        
        if (!api->registerSaveField("player_level", &playerLevel)) return false;
        if (!api->registerSaveField("player_experience", &playerExperience)) return false;
        if (!api->registerSaveField("high_score", &highScore)) return false;
        if (!api->registerSaveField("game_volume", &gameVolume)) return false;
        if (!api->registerSaveField("tutorial_completed", &tutorialCompleted)) return false;
        if (!api->registerSaveField("player_name", &playerName, 32)) return false;
        if (!api->registerSaveBlob("settings_data", settingsData, sizeof(settingsData))) return false;
        
        api->print("Save fields registered successfully");
        return true;
    }
    
    void changePlayerName() {
        // Cycle through some predefined names for demo
        static const char* names[] = {"Player", "Hero", "Champion", "Legend", "Master"};
        static uint8_t nameIndex = 0;
        
        nameIndex = (nameIndex + 1) % 5;
        playerName = names[nameIndex];
        
        api->print("Player name changed to: " + playerName);
    }
    
    void resetProgress() {
        // Reset all progress to defaults
        playerLevel = 1;
        playerExperience = 0;
        highScore = 0;
        gameVolume = 0.8f;
        tutorialCompleted = false;
        playerName = "Player";
        memset(settingsData, 0, sizeof(settingsData));
        settingsData[0] = 0xFF;
        
        // Delete save file
        if (api->deleteSaveFile()) {
            api->print("Progress reset - save file deleted");
        } else {
            api->printWarning("Progress reset but save file deletion failed");
        }
    }
};

// Factory function for creating the app
extern "C" WispAppBase* createSaveDemoApp() {
    return new SaveDemoApp();
}

extern "C" void destroySaveDemoApp(WispAppBase* app) {
    delete app;
}
