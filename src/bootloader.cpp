// bootloader.cpp - Wisp Engine Native ESP32-C6/S3 Bootloader using ESP-IDF
// 
// ESP32 ARCHITECTURE NOTES:
// - Uses ESP-IDF framework (NOT Arduino) with compatibility layer
// - Targets ESP32-C6 (512KB HP-SRAM, 16KB LP-SRAM) and ESP32-S3 variants
// - All timing uses esp_timer, all GPIO uses native ESP-IDF drivers
// - Arduino compatibility provided through esp32_common.h
// 
// Clean micro-folder architecture with namespace organization

// ESP-IDF includes
// Note: SPIFFS includes removed for ESP-IDF v6+ compatibility

// Core engine with clean namespaces
#include "engine/namespaces.h"
#include "engine/graphics/engine.h"  // For SCREEN_WIDTH/HEIGHT constants

#ifdef CONFIG_IDF_TARGET_ESP32C6
#include "engine/minimal/minimal_engine.h"  // Minimal engine for ESP32-C6
#include "engine/minimal/minimal_api_wrapper.h"  // Minimal API wrapper
#include "engine/database/unified_database.h"  // Unified database system
#endif

// System components (existing)
#include "system/esp32_common.h"
#include "system/definitions.h"
#include "system/app_manager.h"
#include "system/input_controller.h"
#ifndef CONFIG_IDF_TARGET_ESP32C6
#include "system/ui/panels/menu.h"
#endif
#include "system/display_driver.h"  // For LGFX class
#include "core/timekeeper.h"

// External libraries
#include <LovyanGFX.hpp>

// Namespace organization - avoid ambiguity by not importing full WispEngine namespace
using namespace WispEngine::Graphics;  // For SCREEN_WIDTH/HEIGHT constants

// --- GLOBAL ENGINE COMPONENTS ---
LGFX display;  // Uses LGFX from display_driver.h

#ifdef CONFIG_IDF_TARGET_ESP32C6
// ESP32-C6: Use minimal engine
WispEngine::Minimal::Engine mainEngine;  // Minimal engine instance
WispEngine::Minimal::APIWrapper curatedAPI(&mainEngine);  // Minimal API wrapper
#else
// ESP32-S3: Use full engine
WispEngine::Engine mainEngine;  // Full engine instance  
WispCuratedAPI curatedAPI(&mainEngine);  // Use existing WispCuratedAPI with engine
#endif

AppLoader appLoader;
AppLoopManager appLoop;  // Add missing appLoop variable
AppManager appManager;
InputController* inputController;

// Boot state
bool bootComplete = false;
bool menuInitialized = false;

// Forward declarations
WispInputState convertToWispInput();
void printPerformanceStats();
void handleCriticalError(const String& error);

// --- SETUP PHASE ---
void bootloaderSetup() {
    // Initialize debug system first (use proper enum values)
    WispEngine::Core::Debug::init(WispEngine::Core::Debug::DEBUG_MODE_ON, 
                                  WispEngine::Core::Debug::SAFETY_MODE_DISABLED);
    
    // NOW we can use WISP_DEBUG - all logging must go through the main system
    WISP_DEBUG_INFO("WISP", "=== Wisp Engine Native C++ Bootloader ===");
    WISP_DEBUG_INFO("WISP", "Starting boot sequence...");
    WISP_DEBUG_INFO("WISP", "Debug system initialized");
    
    // Initialize display first
    display.init();
    display.setBrightness(255);
    display.setColorDepth(16);
    display.fillScreen(0x0000); // Black
    
    WISP_DEBUG_INFO("WISP", "Display initialized");
    
    // Show initial boot message
    display.setTextColor(0xFFFF);
    display.setTextDatum(top_center);
    display.drawString("WISP ENGINE", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20);
    display.drawString("Booting...", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    
    // Initialize storage systems (ESP-IDF v6+ compatible)
    // Note: In ESP-IDF v6+, SPIFFS is typically auto-mounted via partition table
    // For now, skip SPIFFS initialization as it's not critical for basic bootloader
    WISP_DEBUG_INFO("WISP", "Skipping SPIFFS initialization for ESP-IDF v6+ compatibility");
    
    // Try to initialize SD card (ESP-IDF native, non-breaking, optional)
    bool sdAvailable = false;
    WISP_DEBUG_INFO("WISP", "Attempting SD card initialization...");
    // TODO: Implement proper ESP-IDF SD card initialization
    // For now, assume SD not available - use SPIFFS only
    if (sdAvailable) {
        WISP_DEBUG_INFO("WISP", "SD card initialized successfully");
    } else {
        WISP_DEBUG_INFO("WISP", "SD card not available - using SPIFFS only");
    }
    
    WISP_DEBUG_INFO("WISP", "Save system skipped for basic bootloader test");
    
    // Initialize input controller
    inputController = new InputController();
    if (!inputController->init()) {
        WISP_DEBUG_ERROR("WISP", "Input controller initialization failed");
        while(1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Initialize timing system (use namespace bridge)
    WispEngine::Core::Timing::init();
    
#ifdef CONFIG_IDF_TARGET_ESP32C6
    // Initialize minimal engine for ESP32-C6
    if (!mainEngine.init()) {
        WISP_DEBUG_ERROR("WISP", "Minimal engine initialization failed");
        display.fillScreen(0xF800); // Red error screen
        display.setTextColor(0xFFFF);
        display.drawString("MINIMAL ENGINE ERROR", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        while(1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    WISP_DEBUG_INFO("WISP", "Minimal engine initialized successfully");
#endif
    
    // Initialize app manager (use existing)
    if (!appManager.init(&appLoader, &appLoop)) {
        WISP_DEBUG_ERROR("WISP", "App manager initialization failed");
        display.fillScreen(0xF800); // Red error screen
        display.setTextColor(0xFFFF);
        display.drawString("APP MANAGER ERROR", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        while(1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Scan for available .wisp applications
    appManager.scanForApps();
    
    WISP_DEBUG_INFO("WISP", "App manager initialized successfully");
}

#ifdef CONFIG_IDF_TARGET_ESP32C6
// Test the WBDF structured database system
void testUnifiedDatabase() {
    static bool dbTested = false;
    if (dbTested) return;  // Only test once
    
    ESP_LOGI("DB", "Testing unified database system...");
    
    // Initialize with 8KB memory
    WispErrorCode result = wispDB.initialize(8192);
    if (result != WISP_SUCCESS) {
        ESP_LOGE("DB", "Failed to initialize unified database: %d", result);
        return;
    }
    
    ESP_LOGI("DB", "✓ Unified database initialized");
    
    // === TEST KEY-VALUE STORE ===
    wispDB.setU32(0x12345678, 42);
    wispDB.setString(0x12345679, "Test String");
    
    uint32_t val = wispDB.getU32(0x12345678);
    char strBuffer[32];
    wispDB.getString(0x12345679, strBuffer, sizeof(strBuffer));
    
    ESP_LOGI("DB", "Key-value test: %d, '%s'", (int)val, strBuffer);
    
    // === TEST STRUCTURED TABLES ===
    const WBDFColumn itemColumns[] = {
        WBDF_PRIMARY_KEY("id", WBDF_TYPE_U16),
        WBDF_COLUMN("name", WBDF_TYPE_STRING, 24),
        WBDF_COLUMN("category", WBDF_TYPE_U8, 0),
        WBDF_COLUMN("value", WBDF_TYPE_U32, 0)
    };
    
    uint16_t itemTableId = wispDB.createTable("items", itemColumns, 4, 32, WBDF_TABLE_READ_WRITE);
    ESP_LOGI("DB", "Created items table: %d", itemTableId);
    
    // Create read-only NPCs table
    const WBDFColumn npcColumns[] = {
        WBDF_PRIMARY_KEY("id", WBDF_TYPE_U16),
        WBDF_COLUMN("name", WBDF_TYPE_STRING, 20),
        WBDF_COLUMN("level", WBDF_TYPE_U8, 0)
    };
    
    uint16_t npcTableId = wispDB.createTable("npcs", npcColumns, 3, 16, WBDF_TABLE_READ_ONLY);
    ESP_LOGI("DB", "Created NPCs table: %d (read-only)", npcTableId);
    
    // Test table permissions
    uint8_t itemPerms = wispDB.getTablePermissions(itemTableId);
    uint8_t npcPerms = wispDB.getTablePermissions(npcTableId);
    ESP_LOGI("DB", "Permissions - Items: 0x%02X, NPCs: 0x%02X", itemPerms, npcPerms);
    
    // Insert items (should work)
    struct ItemRow {
        uint16_t id;
        char name[24];
        uint8_t category;
        uint32_t value;
    } __attribute__((packed));
    
    ItemRow sword = {1, "Iron Sword", 1, 100};
    ItemRow potion = {2, "Health Potion", 3, 50};
    
    uint16_t row1 = wispDB.insertRow(itemTableId, &sword);
    uint16_t row2 = wispDB.insertRow(itemTableId, &potion);
    ESP_LOGI("DB", "Inserted items: rows %d, %d", row1, row2);
    
    // Try to insert into read-only table (should fail)
    struct NPCRow {
        uint16_t id;
        char name[20];
        uint8_t level;
    } __attribute__((packed));
    
    NPCRow npc = {1, "Elder", 50};
    uint16_t npcRow = wispDB.insertRow(npcTableId, &npc);
    ESP_LOGI("DB", "Tried NPC insert (read-only): row %d (should be 0)", npcRow);
    
    // Make NPCs writable and try again
    wispDB.setTablePermissions(npcTableId, WBDF_TABLE_READ_WRITE);
    uint16_t npcRow2 = wispDB.insertRow(npcTableId, &npc);
    ESP_LOGI("DB", "NPC insert after making writable: row %d", npcRow2);
    
    // Print stats
    ESP_LOGI("DB", "Memory: %d/%d bytes used", 
             (int)wispDB.getUsedMemory(), 8192);
    
    if (wispDB.validateDatabase()) {
        ESP_LOGI("DB", "✓ Database validation passed");
    }
    
    ESP_LOGI("DB", "✓ Unified database test complete");
    dbTested = true;
}
#endif

// --- MAIN LOOP ---
void bootloaderLoop() {
    // Frame timing control (use namespace bridge)
    if (!WispEngine::Core::Timing::frameReady()) return;
    WispEngine::Core::Timing::tick();
    
    // System heartbeat (use namespace bridge)
    WispEngine::Core::Debug::heartbeat();
    
    // Update input
    inputController->update();
    WispInputState inputState = convertToWispInput();  // Use existing input state
    
    if (!bootComplete) {
        // Simple boot - just mark as complete after initialization
        bootComplete = true;
        
        // Initialize menu system (use existing WispMenu)
        if (!menuInitialized) {
#ifdef CONFIG_IDF_TARGET_ESP32C6
            // ESP32-C6: Minimal mode - skip menu for now
            menuInitialized = true;
            Serial.println("ESP32-C6 minimal mode - menu disabled");
#else
            if (WispMenu::init(&curatedAPI)) {
                // Grant menu system permission to launch apps (it's a system component)
                curatedAPI.setAppPermissions(true, false, false, false);
                
                menuInitialized = true;
                WISP_DEBUG_INFO("WISP", "Menu system initialized with app launch permissions");
            } else {
                WISP_DEBUG_ERROR("WISP", "Menu system initialization failed");
            }
#endif
        }
        
#ifndef CONFIG_IDF_TARGET_ESP32C6
        // Show menu with available .wisp apps
        WispMenu::activate();
#endif
        
    } else {
        // Post-boot: either menu or app is running
        
#ifdef CONFIG_IDF_TARGET_ESP32C6
        // ESP32-C6: Enhanced GBA-style demo
        static int demoFrame = 0;
        static int spriteX = 50, spriteY = 50;
        static int spriteVelX = 2, spriteVelY = 1;
        static bool tilesLoaded = false;
        
        // Update minimal engine
        mainEngine.update();
        
        // Initialize GBA-style features on first run
        if (!tilesLoaded) {
            // Set up a simple palette
            mainEngine.graphics().setPaletteColor(0, 0x0000);   // Black (transparent)
            mainEngine.graphics().setPaletteColor(1, 0x001F);   // Blue
            mainEngine.graphics().setPaletteColor(2, 0x03E0);   // Green  
            mainEngine.graphics().setPaletteColor(3, 0x7C00);   // Red
            mainEngine.graphics().setPaletteColor(4, 0xFFE0);   // Yellow
            mainEngine.graphics().setPaletteColor(5, 0xF81F);   // Magenta
            mainEngine.graphics().setPaletteColor(6, 0x07FF);   // Cyan
            mainEngine.graphics().setPaletteColor(7, 0xFFFF);   // White
            
            // Load some demo tiles for background
            mainEngine.graphics().loadTile(1, nullptr, 1);  // Blue checkerboard tile
            mainEngine.graphics().loadTile(2, nullptr, 2);  // Green checkerboard tile
            
            // Set up a simple background pattern
            for (int y = 0; y < 12; y++) {
                for (int x = 0; x < 20; x++) {
                    uint8_t tileId = ((x + y) % 2) ? 1 : 2;
                    mainEngine.graphics().setTile(0, x, y, tileId);
                }
            }
            mainEngine.graphics().setBackgroundEnabled(0, true);
            
            tilesLoaded = true;
        }
        
        // Clear screen with palette color
        mainEngine.graphics().clear(mainEngine.graphics().getPaletteColor(1));  // Blue background
        
        // Draw UI text
        mainEngine.graphics().drawText(10, 10, "WISP GBA-STYLE ENGINE", 0xFFFF);
        mainEngine.graphics().drawText(10, 30, "ESP32-C6 Enhanced Mode", 0xF81F);
        
        // Animate background scrolling (GBA-style)
        static int scrollX = 0;
        scrollX = (scrollX + 1) % 64;  // Scroll every 64 pixels
        mainEngine.graphics().scrollBackground(0, scrollX, 0);
        
        // Draw moving sprites with enhanced features
        spriteX += spriteVelX;
        spriteY += spriteVelY;
        if (spriteX <= 0 || spriteX >= 200) spriteVelX = -spriteVelX;
        if (spriteY <= 60 || spriteY >= 140) spriteVelY = -spriteVelY;
        
        // Use enhanced sprite drawing with flipping and priority
        bool flipX = spriteVelX < 0;  // Flip sprite based on direction
        mainEngine.graphics().drawSprite(1, spriteX, spriteY, 1, flipX, false, 1);      // Player sprite
        mainEngine.graphics().drawSprite(2, spriteX + 30, spriteY + 20, 1, !flipX, false, 1); // Enemy sprite
        mainEngine.graphics().drawSprite(3, 100, 80, 1, false, false, 2); // Collectible (higher priority)
        
        // Draw frame counter and memory info
        char frameText[64];
        sprintf(frameText, "Frame: %d | Scroll: %d", demoFrame++, scrollX);
        mainEngine.graphics().drawText(10, 50, frameText, 0x07E0);
        
        sprintf(frameText, "Free: %lu KB | Tiles: %d", ESP.getFreeHeap() / 1024, mainEngine.graphics().getTileCount());
        mainEngine.graphics().drawText(10, 200, frameText, 0xFFE0);
        
        // Test multi-channel audio (GBA-style)
        if (demoFrame % 60 == 0) {  // Every second
            mainEngine.audio().playNote(WispEngine::Minimal::CHANNEL_SQUARE1, 440, 8, 30);  // A note on square channel 1
        }
        if (demoFrame % 120 == 30) {  // Half second offset
            mainEngine.audio().playNote(WispEngine::Minimal::CHANNEL_SQUARE2, 880, 6, 20);  // High A on square channel 2
        }
        
        // Test unified database every 10 seconds
        if (demoFrame % 600 == 0) {
            testUnifiedDatabase();
        }
        
        mainEngine.graphics().display();
#else
        if (WispMenu::isActive()) {
            // Menu is active
            WispMenu::update(inputState);
            WispMenu::render();
            
        } else if (appManager.isAppRunning()) {
            // App is running via app manager
            appManager.update();
            
        } else {
            // No app or menu - show error or return to menu
            WISP_DEBUG_INFO("WISP", "No active app or menu - returning to menu");
            WispMenu::activate();
        }
#endif
    }
    
#ifndef CONFIG_IDF_TARGET_ESP32C6
    // Emergency menu activation (hold SELECT + BACK)
    static uint32_t emergencyMenuTimer = 0;
    if (inputState.select && inputState.buttonB) {
        if (emergencyMenuTimer == 0) {
            emergencyMenuTimer = get_millis();
        } else if (get_millis() - emergencyMenuTimer > 2000) {
            // 2 seconds of holding both buttons
            WISP_DEBUG_INFO("WISP", "Emergency menu activation");
            WispMenu::activate();
            emergencyMenuTimer = 0;
        }
    } else {
        emergencyMenuTimer = 0;
    }
#endif
    
    // Performance monitoring
    static uint32_t lastStatsTime = 0;
    if (get_millis() - lastStatsTime > 5000) { // Every 5 seconds
        printPerformanceStats();
        lastStatsTime = get_millis();
    }
}

// --- UTILITY FUNCTIONS ---
WispInputState convertToWispInput() {
    WispInputState state;
    
    // Map hardware buttons to Wisp input state
    state.left = inputController->isPressed(BTN_LEFT);
    state.right = inputController->isPressed(BTN_RIGHT);
    state.up = inputController->isPressed(BTN_UP);
    state.down = inputController->isPressed(BTN_DOWN);
    state.buttonA = inputController->isPressed(BTN_A);
    state.buttonB = inputController->isPressed(BTN_B);
    state.buttonC = false; // Not available on this hardware
    state.select = inputController->isPressed(BTN_A);
    state.start = false; // Not available on this hardware
    
    // Analog input (if available)
    state.analogX = 0;
    state.analogY = 0;
    
    // Touch input (if available)
    state.touched = false;
    state.touchX = 0;
    state.touchY = 0;
    
    return state;
}

bool launchApp(const String& appPath) {
    WISP_DEBUG_INFO("WISP", "Launching app");
    
    // Use app manager to load the .wisp file (existing system)
    if (appManager.loadApp(appPath)) {
        WISP_DEBUG_INFO("WISP", "App launched successfully via app manager");
        return true;
    } else {
        WISP_DEBUG_ERROR("WISP", "App launch failed");
        return false;
    }
}

void printPerformanceStats() {
    uint32_t freeHeap = esp_get_free_heap_size();
    WISP_DEBUG_INFO("STATS", "FPS and Memory Statistics");
    WISP_DEBUG_INFO("STATS", String("Free heap: " + String(freeHeap) + " bytes").c_str());
}

// Emergency error handler
void handleCriticalError(const String& error) {
    WISP_DEBUG_ERROR("WISP", "CRITICAL ERROR");
    
    // Activate emergency mode in debug system (use namespace bridge)
    WispEngine::Core::Debug::activateEmergencyMode(error);
    
    // Show error on display
    display.fillScreen(0xF800); // Red background
    display.setTextColor(0xFFFF);
    display.setTextDatum(top_center);
    display.drawString("SYSTEM ERROR", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 40);
    display.drawString(error, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 10);
    display.drawString("Hold RESET to restart", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20);
    
    // Stop all systems
    appManager.stopApp();
    
    // Shutdown debug system to flush logs
    WispEngine::Core::Debug::shutdown();
    
    // Infinite loop until reset
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
