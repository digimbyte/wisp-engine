// examples/cartridge_system_example.cpp
#include <Arduino.h>
#include <SPIFFS.h>
#include "../src/engine/wisp_cartridge_system.h"
#include "../src/apps/test_cartridge_app.h"

/**
 * WISP Cartridge System Example
 * Demonstrates GBA-like ROM loading and execution
 */

void setup() {
    Serial.begin(115200);
    Serial.println("WISP Cartridge System Example");
    Serial.println("=============================");
    
    // Initialize SPIFFS for save data
    if (!SPIFFS.begin(true)) {
        Serial.println("ERROR: Could not initialize SPIFFS");
        return;
    }
    
    // Initialize cartridge system
    g_CartridgeSystem = new WispCartridgeSystem();
    if (!g_CartridgeSystem) {
        Serial.println("ERROR: Could not create cartridge system");
        return;
    }
    
    // Set memory budget (128KB for this example)
    g_CartridgeSystem->setMemoryBudget(131072);
    
    Serial.println("Cartridge system initialized");
    Serial.println();
    
    // Try to insert a ROM cartridge
    Serial.println("Attempting to insert ROM: /roms/test_app.wisp");
    
    if (g_CartridgeSystem->insertCartridge("/roms/test_app.wisp")) {
        Serial.println("ROM cartridge inserted successfully!");
        
        // Print cartridge info
        const CartridgeInfo& info = g_CartridgeSystem->getCartridgeInfo();
        Serial.print("Title: "); Serial.println(info.title);
        Serial.print("Version: "); Serial.println(info.version);
        Serial.print("Author: "); Serial.println(info.author);
        Serial.print("Description: "); Serial.println(info.description);
        Serial.print("Assets: "); Serial.println(info.assetCount);
        Serial.print("Target FPS: "); Serial.println(info.targetFPS);
        Serial.print("Required RAM: "); Serial.println(info.requiredRAM);
        Serial.println();
        
        // Boot the ROM
        Serial.println("Booting ROM...");
        if (g_CartridgeSystem->bootROM()) {
            Serial.println("ROM booted successfully!");
        } else {
            Serial.println("ERROR: Could not boot ROM");
        }
    } else {
        Serial.println("ERROR: Could not insert ROM cartridge");
        Serial.println("Creating test ROM with sample data...");
        
        // Create a simple test without actual ROM file
        createTestROM();
    }
}

void loop() {
    static uint32_t lastUpdate = 0;
    static uint32_t lastPerformanceReport = 0;
    
    uint32_t currentTime = millis();
    
    // Update cartridge system
    if (g_CartridgeSystem) {
        uint32_t deltaTime = currentTime - lastUpdate;
        lastUpdate = currentTime;
        
        g_CartridgeSystem->update(deltaTime);
        
        // Print performance stats every 10 seconds
        if (currentTime - lastPerformanceReport >= 10000) {
            g_CartridgeSystem->printPerformanceStats();
            lastPerformanceReport = currentTime;
        }
        
        // Handle serial commands
        handleSerialCommands();
    }
    
    delay(16);  // ~60 FPS main loop
}

void createTestROM() {
    Serial.println("Creating test ROM in memory...");
    
    // Create cartridge info manually
    CartridgeInfo testInfo;
    testInfo.title = "Memory Test ROM";
    testInfo.version = "1.0.0";
    testInfo.author = "WISP Engine";
    testInfo.description = "Test ROM created in memory";
    testInfo.assetCount = 2;
    testInfo.targetFPS = 16;
    testInfo.requiredRAM = 32768;
    testInfo.needsWiFi = false;
    testInfo.needsBluetooth = false;
    testInfo.needsEEPROM = false;
    testInfo.validated = true;
    
    // Create test app directly
    TestCartridgeApp* testApp = new TestCartridgeApp();
    if (testApp && testApp->internalInit()) {
        Serial.println("Test app created and initialized");
        
        // Simulate running the app
        for (int i = 0; i < 10; i++) {
            testApp->internalUpdate(16);
            testApp->internalRender();
            delay(100);
        }
        
        testApp->printStats();
        testApp->internalCleanup();
        delete testApp;
        
        Serial.println("Test app completed");
    } else {
        Serial.println("ERROR: Could not create test app");
    }
}

void handleSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command == "stats") {
            if (g_CartridgeSystem) {
                g_CartridgeSystem->printPerformanceStats();
            }
        }
        else if (command == "info") {
            if (g_CartridgeSystem && g_CartridgeSystem->getCurrentState() != CARTRIDGE_EMPTY) {
                const CartridgeInfo& info = g_CartridgeSystem->getCartridgeInfo();
                Serial.println("=== CARTRIDGE INFO ===");
                Serial.print("Title: "); Serial.println(info.title);
                Serial.print("Version: "); Serial.println(info.version);
                Serial.print("Author: "); Serial.println(info.author);
                Serial.print("Description: "); Serial.println(info.description);
                Serial.print("Assets: "); Serial.println(info.assetCount);
                Serial.print("ROM Size: "); Serial.println(info.romSize);
                Serial.print("Target FPS: "); Serial.println(info.targetFPS);
                Serial.print("Required RAM: "); Serial.println(info.requiredRAM);
                Serial.print("Needs WiFi: "); Serial.println(info.needsWiFi ? "Yes" : "No");
                Serial.print("Needs Bluetooth: "); Serial.println(info.needsBluetooth ? "Yes" : "No");
                Serial.print("Needs EEPROM: "); Serial.println(info.needsEEPROM ? "Yes" : "No");
                Serial.println("======================");
            } else {
                Serial.println("No cartridge inserted");
            }
        }
        else if (command == "reset") {
            if (g_CartridgeSystem) {
                Serial.println("Resetting ROM...");
                g_CartridgeSystem->resetROM();
            }
        }
        else if (command == "eject") {
            if (g_CartridgeSystem) {
                Serial.println("Ejecting cartridge...");
                g_CartridgeSystem->ejectCartridge();
            }
        }
        else if (command == "power") {
            if (g_CartridgeSystem) {
                Serial.println("Powering off ROM...");
                g_CartridgeSystem->powerOff();
            }
        }
        else if (command == "help") {
            Serial.println("Available commands:");
            Serial.println("  stats  - Show performance statistics");
            Serial.println("  info   - Show cartridge information");
            Serial.println("  reset  - Reset the current ROM");
            Serial.println("  eject  - Eject the cartridge");
            Serial.println("  power  - Power off the ROM");
            Serial.println("  help   - Show this help");
        }
        else if (command.length() > 0) {
            Serial.print("Unknown command: ");
            Serial.println(command);
            Serial.println("Type 'help' for available commands");
        }
    }
}
