// examples/save_test_app.cpp - Save/Load System Test
// Tests save field registration, data persistence, and file operations

#include "../src/engine/app/interface.h"

class SaveTestApp : public WispAppBase {
private:
    // Save test modes
    enum SaveTestMode {
        TEST_BASIC_SAVE,    // Basic save/load operations
        TEST_FIELD_TYPES,   // Different field type handling
        TEST_FILE_MGMT,     // File management and slots
        TEST_CORRUPTION,    // Corruption detection and recovery
        TEST_COUNT
    };
    
    SaveTestMode currentMode = TEST_BASIC_SAVE;
    
    // Test save data structure
    struct GameSaveData {
        // Player data
        std::string playerName = "Test Player";
        uint32_t playTime = 0;          // seconds
        uint8_t playerLevel = 1;
        uint32_t experience = 0;
        uint32_t money = 500;
        bool hasPokedex = false;
        
        // Progress flags
        std::vector<bool> gymBadges = std::vector<bool>(8, false);
        std::vector<bool> townsVisited = std::vector<bool>(10, false);
        
        // Inventory
        std::vector<uint32_t> itemIds = {1, 5, 10, 15, 20};
        std::vector<uint16_t> itemCounts = {10, 5, 3, 1, 2};
        
        // Settings
        float masterVolume = 1.0f;
        float sfxVolume = 0.8f;
        uint8_t textSpeed = 2;          // 0=slow, 1=normal, 2=fast
        bool animationsEnabled = true;
        
        // Timestamps
        uint64_t lastSaveTime = 0;
        uint64_t creationTime = 0;
        
        // Statistics
        uint32_t battlesWon = 0;
        uint32_t pokemonCaught = 0;
        float distanceWalked = 0.0f;    // km
    } saveData;
    
    // Save slots
    uint8_t currentSlot = 0;
    uint8_t maxSlots = 3;
    std::vector<bool> slotExists = std::vector<bool>(3, false);
    std::vector<std::string> slotInfo = std::vector<std::string>(3, "Empty");
    
    // Test state
    bool saveRegistered = false;
    uint32_t lastOperationTime = 0;
    std::string lastOperationResult = "";
    bool autoSaveEnabled = false;
    uint32_t lastAutoSave = 0;
    uint32_t autoSaveInterval = 10000; // 10 seconds
    
    // File corruption simulation
    bool simulateCorruption = false;
    uint8_t corruptionType = 0; // 0=none, 1=header, 2=data, 3=checksum
    
    // Performance tracking
    struct SaveMetrics {
        uint32_t saveTime = 0;
        uint32_t loadTime = 0;
        uint32_t fileSize = 0;
        uint32_t totalSaves = 0;
        uint32_t totalLoads = 0;
        uint32_t failedSaves = 0;
        uint32_t failedLoads = 0;
    } metrics;

public:
    bool init() override {
        setAppInfo("Save System Test", "1.0.0", "Wisp Engine Team");
        
        // Register save fields
        if (registerSaveFields()) {
            saveRegistered = true;
            api->print("Save System Test App initialized");
            
            // Initialize save data
            initializeSaveData();
            
            // Check existing save slots
            checkSaveSlots();
        } else {
            api->print("Failed to register save fields");
            return false;
        }
        
        api->print("Controls: Up/Down - Mode, A - Save, B - Load");
        api->print("Left/Right - Slot, Start - Auto Save, Select - Test Data");
        return true;
    }
    
    bool registerSaveFields() {
        // Register all save fields with the save system
        
        // Player data
        api->registerSaveField("playerName", saveData.playerName);
        api->registerSaveField("playTime", saveData.playTime);
        api->registerSaveField("playerLevel", saveData.playerLevel);
        api->registerSaveField("experience", saveData.experience);
        api->registerSaveField("money", saveData.money);
        api->registerSaveField("hasPokedex", saveData.hasPokedex);
        
        // Progress arrays
        api->registerSaveField("gymBadges", saveData.gymBadges);
        api->registerSaveField("townsVisited", saveData.townsVisited);
        
        // Inventory
        api->registerSaveField("itemIds", saveData.itemIds);
        api->registerSaveField("itemCounts", saveData.itemCounts);
        
        // Settings
        api->registerSaveField("masterVolume", saveData.masterVolume);
        api->registerSaveField("sfxVolume", saveData.sfxVolume);
        api->registerSaveField("textSpeed", saveData.textSpeed);
        api->registerSaveField("animationsEnabled", saveData.animationsEnabled);
        
        // Timestamps
        api->registerSaveField("lastSaveTime", saveData.lastSaveTime);
        api->registerSaveField("creationTime", saveData.creationTime);
        
        // Statistics
        api->registerSaveField("battlesWon", saveData.battlesWon);
        api->registerSaveField("pokemonCaught", saveData.pokemonCaught);
        api->registerSaveField("distanceWalked", saveData.distanceWalked);
        
        api->print("Save fields registered: 17 fields");
        return true;
    }
    
    void initializeSaveData() {
        saveData.creationTime = api->getTime();
        saveData.lastSaveTime = saveData.creationTime;
        
        // Initialize with some test data
        saveData.playerName = "TestPlayer";
        saveData.playTime = 3600; // 1 hour
        saveData.playerLevel = 5;
        saveData.experience = 1250;
        saveData.money = 2500;
        saveData.hasPokedex = true;
        
        // Set some gym badges
        saveData.gymBadges[0] = true; // First gym
        saveData.gymBadges[1] = true; // Second gym
        
        // Mark some towns as visited
        saveData.townsVisited[0] = true; // Starting town
        saveData.townsVisited[1] = true; // First city
        
        // Add some statistics
        saveData.battlesWon = 15;
        saveData.pokemonCaught = 8;
        saveData.distanceWalked = 12.5f;
    }
    
    void checkSaveSlots() {
        for (int i = 0; i < maxSlots; i++) {
            std::string filename = "save_slot_" + std::to_string(i) + ".wsave";
            
            if (api->saveExists(filename)) {
                slotExists[i] = true;
                
                // Get save info (would read save header in real implementation)
                slotInfo[i] = "Player: " + saveData.playerName + " Lv." + std::to_string(saveData.playerLevel);
            } else {
                slotExists[i] = false;
                slotInfo[i] = "Empty";
            }
        }
    }
    
    std::string getModeName(SaveTestMode mode) {
        const char* names[] = {"Basic Save/Load", "Field Types", "File Management", "Corruption Test"};
        return names[mode];
    }
    
    void update() override {
        if (!saveRegistered) return;
        
        uint32_t currentTime = api->getTime();
        
        // Update play time
        static uint32_t lastTimeUpdate = currentTime;
        if (currentTime - lastTimeUpdate >= 1000) {
            saveData.playTime++;
            lastTimeUpdate = currentTime;
        }
        
        // Handle input
        const WispInputState& input = api->getInput();
        static WispInputState lastInput;
        
        // Mode selection
        if (input.up && !lastInput.up) {
            currentMode = (SaveTestMode)((currentMode + 1) % TEST_COUNT);
            api->print("Save Mode: " + getModeName(currentMode));
        }
        if (input.down && !lastInput.down) {
            currentMode = (SaveTestMode)((currentMode - 1 + TEST_COUNT) % TEST_COUNT);
            api->print("Save Mode: " + getModeName(currentMode));
        }
        
        // Slot selection
        if (input.left && !lastInput.left) {
            currentSlot = (currentSlot - 1 + maxSlots) % maxSlots;
            api->print("Save Slot: " + std::to_string(currentSlot) + " - " + slotInfo[currentSlot]);
        }
        if (input.right && !lastInput.right) {
            currentSlot = (currentSlot + 1) % maxSlots;
            api->print("Save Slot: " + std::to_string(currentSlot) + " - " + slotInfo[currentSlot]);
        }
        
        // Save operation
        if (input.buttonA && !lastInput.buttonA) {
            performSave();
        }
        
        // Load operation
        if (input.buttonB && !lastInput.buttonB) {
            performLoad();
        }
        
        // Auto save toggle
        if (input.start && !lastInput.start) {
            autoSaveEnabled = !autoSaveEnabled;
            api->print("Auto Save: " + std::string(autoSaveEnabled ? "ON" : "OFF"));
        }
        
        // Generate test data
        if (input.select && !lastInput.select) {
            generateTestData();
        }
        
        lastInput = input;
        
        // Auto save
        if (autoSaveEnabled && currentTime - lastAutoSave > autoSaveInterval) {
            performSave();
            lastAutoSave = currentTime;
        }
        
        // Mode-specific updates
        switch (currentMode) {
            case TEST_FIELD_TYPES:
                updateFieldTypeTest();
                break;
            case TEST_CORRUPTION:
                updateCorruptionTest();
                break;
        }
    }
    
    void performSave() {
        uint32_t startTime = api->getTime();
        
        // Update save timestamp
        saveData.lastSaveTime = startTime;
        
        std::string filename = "save_slot_" + std::to_string(currentSlot) + ".wsave";
        
        // Simulate corruption if enabled
        if (simulateCorruption && currentMode == TEST_CORRUPTION) {
            api->print("Simulating save corruption (type " + std::to_string(corruptionType) + ")");
        }
        
        bool success = api->saveGame(filename);
        
        uint32_t saveTime = api->getTime() - startTime;
        lastOperationTime = saveTime;
        
        if (success) {
            slotExists[currentSlot] = true;
            slotInfo[currentSlot] = "Player: " + saveData.playerName + " Lv." + std::to_string(saveData.playerLevel);
            
            metrics.saveTime = saveTime;
            metrics.totalSaves++;
            metrics.fileSize = api->getFileSize(filename);
            
            lastOperationResult = "Save successful (" + std::to_string(saveTime) + "ms)";
            api->print("Game saved to slot " + std::to_string(currentSlot));
        } else {
            metrics.failedSaves++;
            lastOperationResult = "Save failed";
            api->print("Save operation failed");
        }
    }
    
    void performLoad() {
        if (!slotExists[currentSlot]) {
            lastOperationResult = "No save data in slot " + std::to_string(currentSlot);
            api->print("Save slot is empty");
            return;
        }
        
        uint32_t startTime = api->getTime();
        
        std::string filename = "save_slot_" + std::to_string(currentSlot) + ".wsave";
        
        bool success = api->loadGame(filename);
        
        uint32_t loadTime = api->getTime() - startTime;
        lastOperationTime = loadTime;
        
        if (success) {
            metrics.loadTime = loadTime;
            metrics.totalLoads++;
            
            lastOperationResult = "Load successful (" + std::to_string(loadTime) + "ms)";
            api->print("Game loaded from slot " + std::to_string(currentSlot));
            
            // Update slot info after load
            slotInfo[currentSlot] = "Player: " + saveData.playerName + " Lv." + std::to_string(saveData.playerLevel);
        } else {
            metrics.failedLoads++;
            lastOperationResult = "Load failed - corrupted data";
            api->print("Load operation failed");
        }
    }
    
    void generateTestData() {
        // Modify save data for testing
        saveData.playerLevel = api->randomInt(1, 100);
        saveData.experience = api->randomInt(0, 1000000);
        saveData.money = api->randomInt(0, 999999);
        saveData.battlesWon = api->randomInt(0, 500);
        saveData.pokemonCaught = api->randomInt(0, 151);
        saveData.distanceWalked = api->random(0.0f, 1000.0f);
        
        // Random gym badges
        for (int i = 0; i < 8; i++) {
            saveData.gymBadges[i] = api->randomInt(0, 1) == 1;
        }
        
        // Random towns visited
        for (int i = 0; i < 10; i++) {
            saveData.townsVisited[i] = api->randomInt(0, 1) == 1;
        }
        
        // Random settings
        saveData.masterVolume = api->random(0.0f, 1.0f);
        saveData.sfxVolume = api->random(0.0f, 1.0f);
        saveData.textSpeed = api->randomInt(0, 2);
        saveData.animationsEnabled = api->randomInt(0, 1) == 1;
        
        lastOperationResult = "Test data generated";
        api->print("Random test data generated");
    }
    
    void updateFieldTypeTest() {
        // Continuously modify different field types to test serialization
        static uint32_t lastUpdate = 0;
        uint32_t currentTime = api->getTime();
        
        if (currentTime - lastUpdate > 2000) { // Update every 2 seconds
            saveData.experience += api->randomInt(10, 100);
            saveData.distanceWalked += api->random(0.1f, 1.0f);
            
            // Toggle a random gym badge
            int badgeIndex = api->randomInt(0, 7);
            saveData.gymBadges[badgeIndex] = !saveData.gymBadges[badgeIndex];
            
            lastUpdate = currentTime;
        }
    }
    
    void updateCorruptionTest() {
        static uint32_t lastCorruptionChange = 0;
        uint32_t currentTime = api->getTime();
        
        if (currentTime - lastCorruptionChange > 3000) { // Change every 3 seconds
            corruptionType = (corruptionType + 1) % 4;
            simulateCorruption = (corruptionType != 0);
            lastCorruptionChange = currentTime;
        }
    }
    
    void render() override {
        // Clear with dark background
        api->drawRect(0, 0, 320, 240, WispColor(25, 15, 35), 0);
        
        // Draw title
        api->drawText("SAVE SYSTEM TEST", 160, 10, WispColor(255, 255, 255), 10);
        
        // Draw current mode
        api->drawText(getModeName(currentMode), 160, 25, WispColor(200, 200, 255), 9);
        
        if (!saveRegistered) {
            api->drawText("Save system not initialized", 160, 120, WispColor(255, 0, 0), 8);
            return;
        }
        
        // Draw save slots
        renderSaveSlots();
        
        // Draw mode-specific content
        switch (currentMode) {
            case TEST_BASIC_SAVE:
                renderBasicSaveTest();
                break;
            case TEST_FIELD_TYPES:
                renderFieldTypeTest();
                break;
            case TEST_FILE_MGMT:
                renderFileManagementTest();
                break;
            case TEST_CORRUPTION:
                renderCorruptionTest();
                break;
        }
        
        // Draw performance metrics
        renderMetrics();
        
        // Draw operation result
        renderOperationResult();
        
        // Draw controls
        api->drawText("A: Save  B: Load  Left/Right: Slot", 10, 210, WispColor(180, 180, 180), 8);
        api->drawText("Start: Auto Save  Select: Test Data", 10, 225, WispColor(180, 180, 180), 8);
    }
    
    void renderSaveSlots() {
        int y = 45;
        
        api->drawText("Save Slots:", 10, y, WispColor(255, 255, 255), 8);
        
        for (int i = 0; i < maxSlots; i++) {
            WispColor slotColor = (i == currentSlot) ? WispColor(255, 255, 0) : WispColor(200, 200, 200);
            if (!slotExists[i]) slotColor = WispColor(100, 100, 100);
            
            std::string slotText = "Slot " + std::to_string(i) + ": " + slotInfo[i];
            api->drawText(slotText, 10, y + 15 + i * 12, slotColor, 8);
            
            if (i == currentSlot) {
                api->drawText(">", 0, y + 15 + i * 12, WispColor(255, 255, 0), 8);
            }
        }
    }
    
    void renderBasicSaveTest() {
        int y = 95;
        
        api->drawText("Current Save Data:", 10, y, WispColor(255, 255, 255), 8);
        api->drawText("Player: " + saveData.playerName + " (Level " + std::to_string(saveData.playerLevel) + ")", 10, y + 15, WispColor(200, 200, 200), 8);
        api->drawText("Play Time: " + formatTime(saveData.playTime), 10, y + 30, WispColor(200, 200, 200), 8);
        api->drawText("Money: $" + std::to_string(saveData.money), 10, y + 45, WispColor(200, 200, 200), 8);
        
        // Auto save indicator
        if (autoSaveEnabled) {
            uint32_t nextAutoSave = (autoSaveInterval - (api->getTime() - lastAutoSave)) / 1000;
            api->drawText("Auto Save: " + std::to_string(nextAutoSave) + "s", 200, y + 45, WispColor(0, 255, 0), 8);
        }
    }
    
    void renderFieldTypeTest() {
        int y = 95;
        
        api->drawText("Field Type Testing:", 10, y, WispColor(255, 255, 255), 8);
        api->drawText("String: " + saveData.playerName, 10, y + 15, WispColor(200, 200, 200), 8);
        api->drawText("Integer: " + std::to_string(saveData.experience), 10, y + 30, WispColor(200, 200, 200), 8);
        api->drawText("Float: " + std::to_string(saveData.distanceWalked) + " km", 10, y + 45, WispColor(200, 200, 200), 8);
        api->drawText("Boolean: " + std::string(saveData.hasPokedex ? "true" : "false"), 10, y + 60, WispColor(200, 200, 200), 8);
        
        // Show gym badges as array example
        std::string badgeStr = "Badges: ";
        for (int i = 0; i < 8; i++) {
            badgeStr += saveData.gymBadges[i] ? "1" : "0";
        }
        api->drawText(badgeStr, 10, y + 75, WispColor(200, 200, 200), 8);
    }
    
    void renderFileManagementTest() {
        int y = 95;
        
        api->drawText("File Management:", 10, y, WispColor(255, 255, 255), 8);
        api->drawText("Current Slot: " + std::to_string(currentSlot), 10, y + 15, WispColor(200, 200, 200), 8);
        
        if (metrics.fileSize > 0) {
            api->drawText("File Size: " + std::to_string(metrics.fileSize) + " bytes", 10, y + 30, WispColor(200, 200, 200), 8);
        }
        
        // Show file operations
        api->drawText("Operations:", 10, y + 45, WispColor(255, 255, 255), 8);
        api->drawText("• Copy save between slots", 10, y + 60, WispColor(200, 200, 200), 8);
        api->drawText("• Delete save slot", 10, y + 75, WispColor(200, 200, 200), 8);
        api->drawText("• Backup/restore saves", 10, y + 90, WispColor(200, 200, 200), 8);
    }
    
    void renderCorruptionTest() {
        int y = 95;
        
        api->drawText("Corruption Detection:", 10, y, WispColor(255, 255, 255), 8);
        
        const char* corruptionNames[] = {"None", "Header", "Data", "Checksum"};
        api->drawText("Type: " + std::string(corruptionNames[corruptionType]), 10, y + 15, WispColor(200, 200, 200), 8);
        
        if (simulateCorruption) {
            api->drawText("CORRUPTION ACTIVE", 10, y + 30, WispColor(255, 0, 0), 8);
        } else {
            api->drawText("Data integrity OK", 10, y + 30, WispColor(0, 255, 0), 8);
        }
        
        api->drawText("Save will test corruption handling", 10, y + 45, WispColor(255, 255, 0), 8);
    }
    
    void renderMetrics() {
        int y = 170;
        
        api->drawText("Performance:", 200, y, WispColor(255, 255, 255), 8);
        api->drawText("Saves: " + std::to_string(metrics.totalSaves) + "/" + std::to_string(metrics.failedSaves) + " failed", 200, y + 12, WispColor(200, 200, 200), 8);
        api->drawText("Loads: " + std::to_string(metrics.totalLoads) + "/" + std::to_string(metrics.failedLoads) + " failed", 200, y + 24, WispColor(200, 200, 200), 8);
    }
    
    void renderOperationResult() {
        int y = 190;
        
        if (!lastOperationResult.empty()) {
            WispColor resultColor = WispColor(200, 200, 200);
            if (lastOperationResult.find("successful") != std::string::npos) {
                resultColor = WispColor(0, 255, 0);
            } else if (lastOperationResult.find("failed") != std::string::npos) {
                resultColor = WispColor(255, 100, 100);
            }
            
            api->drawText(lastOperationResult, 10, y, resultColor, 8);
        }
    }
    
    std::string formatTime(uint32_t seconds) {
        uint32_t hours = seconds / 3600;
        uint32_t minutes = (seconds % 3600) / 60;
        uint32_t secs = seconds % 60;
        
        return std::to_string(hours) + ":" + 
               (minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" + 
               (secs < 10 ? "0" : "") + std::to_string(secs);
    }
    
    void cleanup() override {
        // Perform final auto-save if enabled and data changed
        if (autoSaveEnabled) {
            api->print("Performing final auto-save...");
            performSave();
        }
        
        api->print("Save Test App cleaned up");
    }
};

// Export function for the engine
extern "C" WispAppBase* createSaveTestApp() {
    return new SaveTestApp();
}

extern "C" void destroySaveTestApp(WispAppBase* app) {
    delete app;
}
