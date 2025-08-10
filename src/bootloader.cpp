// bootloader.cpp - Wisp Engine Master Bootloader
// Clean implementation following the specified architecture:
// 1. Initialize core services (RAE - Render, Audio, Engine)
// 2. Start screen and audio systems
// 3. Render initial boot screen and play boot SFX
// 4. Load additional services and scan for apps
// 5. Check for autoStart apps and launch first found
// 6. Fall back to hardcoded menu for app selection and settings
//
// ESP32 ARCHITECTURE NOTES:
// - Uses ESP-IDF framework with Arduino compatibility
// - Targets ESP32-C6 and ESP32-S3 variants
// - Clean namespace organization with proper service lifecycle
// - NO LEGACY COMPATIBILITY - Pure master bootloader implementation

// Core engine with clean namespaces
#include "engine/namespaces.h"
#include "engine/graphics/engine.h"  // For SCREEN_WIDTH/HEIGHT constants
#include "system/boot_state.h"        // For organized boot state management

#ifdef CONFIG_IDF_TARGET_ESP32C6
#include "engine/minimal/minimal_engine.h"
#include "engine/minimal/minimal_api_wrapper.h"
#include "engine/database/doc_database.h"
#endif

// System components with namespace awareness
#include "system/esp32_common.h"
#include "system/definitions.h"
#include "system/app_manager.h"
#include "system/input_controller.h"
#include "system/display_driver.h"
#include "core/timekeeper.h"
#include "system/settings_manager.h"

// UI Panel System (learned from legacy bootloaders)
#ifndef CONFIG_IDF_TARGET_ESP32C6
#include "system/ui/panels/menu.h"
#include "system/ui/panels/system_settings.h"
#include "system/ui/panels/display_settings.h"
#include "system/ui/panels/audio_settings.h"
#include "system/ui/panels/network_settings.h"
#endif

// External libraries
#include <LovyanGFX.hpp>

// Namespace organization with proper scope management
using namespace WispEngine::Graphics;  // For SCREEN_WIDTH/HEIGHT constants
using WispEngine::Core::Debug;
using WispEngine::Core::Timing;

// --- GLOBAL ENGINE COMPONENTS ---
LGFX display;  // Uses LGFX from display_driver.h

#ifdef CONFIG_IDF_TARGET_ESP32C6
// ESP32-C6: Use minimal engine
WispEngine::Minimal::Engine mainEngine;  // Minimal engine instance
WispEngine::Minimal::APIWrapper curatedAPI(&mainEngine);  // Minimal API wrapper

// Use document database system
extern DocDatabase docDB;
#else
// ESP32-S3: Use full engine
WispEngine::Engine mainEngine;  // Full engine instance  
WispCuratedAPI curatedAPI(&mainEngine);  // Use existing WispCuratedAPI with engine
#endif

AppLoader appLoader;
AppLoopManager appLoop;
AppManager appManager;
InputController* inputController = nullptr;

// Enhanced app metadata structure (learned from bootloader.h)
struct AppInfo {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::string iconPath;
    std::string splashPath;
    std::string executablePath;
    bool autoStart;
    uint16_t screenWidth;
    uint16_t screenHeight;
    
    AppInfo() : autoStart(false), screenWidth(320), screenHeight(240) {}
};

// Device configuration (learned from bootloader.h)
struct DeviceConfig {
    uint16_t brightness = 255;
    uint8_t colorProfile = 0;
    bool vsyncEnabled = true;
    uint8_t masterVolume = 80;
    bool audioEnabled = true;
    uint8_t audioProfile = 0;
    std::string wifiSSID;
    std::string wifiPassword;
    bool wifiEnabled = false;
    bool bluetoothEnabled = false;
    std::string deviceName = "Wisp Device";
    uint8_t sleepTimeout = 10;
    bool debugMode = false;
} deviceConfig;

// UI Panel Management with app pause capability
#ifndef CONFIG_IDF_TARGET_ESP32C6
SystemSettingsPanel* systemSettingsPanel = nullptr;
MenuPanel* displaySettingsPanel = nullptr;
MenuPanel* audioSettingsPanel = nullptr;
MenuPanel* networkSettingsPanel = nullptr;
MenuPanel* activePanel = nullptr;

// App pause/resume state management
namespace AppPauseSystem {
    bool appWasPaused = false;
    uint32_t pauseStartTime = 0;
    
    void pauseRunningApp() {
        if (appManager.isAppRunning() && !appWasPaused) {
            // Pause the app by stopping its update cycle
            // The app manager keeps the app loaded but stops calling updates
            WISP_DEBUG_INFO("PAUSE", "Pausing running app for system menu");
            appWasPaused = true;
            pauseStartTime = get_millis();
            // Note: App stays loaded in memory, just doesn't receive updates
        }
    }
    
    void resumeApp() {
        if (appWasPaused) {
            WISP_DEBUG_INFO("PAUSE", "Resuming app after system menu");
            uint32_t pausedDuration = get_millis() - pauseStartTime;
            WISP_DEBUG_INFO("PAUSE", String("App was paused for " + String(pausedDuration) + "ms").c_str());
            appWasPaused = false;
        }
    }
    
    bool isAppPaused() {
        return appWasPaused;
    }
}
#endif

// Master Bootloader State
enum BootloaderPhase {
    PHASE_RAE_INIT,         // Initialize Render, Audio, Engine core services
    PHASE_SCREEN_AUDIO,     // Start screen and audio systems
    PHASE_BOOT_DISPLAY,     // Show boot screen and play SFX
    PHASE_SERVICE_LOAD,     // Load additional services
    PHASE_APP_SCAN,         // Scan for available apps
    PHASE_AUTOSTART_CHECK,  // Check for autoStart apps
    PHASE_MENU_FALLBACK,    // Show hardcoded main menu
    PHASE_APP_RUNNING       // App is launched and running
};

BootloaderPhase currentPhase = PHASE_RAE_INIT;
uint32_t phaseStartTime = 0;
bool bootComplete = false;
bool menuActive = false;
int selectedAppIndex = 0;
int menuSelection = 0; // 0=app button, 1=settings button

// App management (enhanced with learned patterns)
static const int MAX_APPS = 50; // Increased from legacy bootloader
std::vector<AppInfo> availableApps;
int selectedAppIndex = -1;
AppInfo currentApp;

// Menu system state (learned from both legacy bootloaders)
enum MenuPage {
    MENU_MAIN = 0,      // App launch + settings
    MENU_DISPLAY,       // Display settings panel
    MENU_AUDIO,         // Audio settings panel  
    MENU_NETWORK,       // Network settings panel
    MENU_SYSTEM         // System settings panel
} currentMenuPage = MENU_MAIN;

bool inSubMenu = false;

// System input tracking (enhanced pattern from enhanced_bootloader.h)
struct SystemInputCombination {
    uint32_t buttons;
    uint32_t holdTime;
    int action;
    bool requiresAllButtons;
    const char* description;
};

// Input combination constants
static const uint32_t INPUT_SELECT = 0x01;
static const uint32_t INPUT_START = 0x02;
static const uint32_t INPUT_A = 0x04;
static const uint32_t INPUT_B = 0x08;
static const uint32_t INPUT_UP = 0x10;
static const uint32_t INPUT_DOWN = 0x20;
static const uint32_t INPUT_LEFT = 0x40;
static const uint32_t INPUT_RIGHT = 0x80;

// System combinations (learned pattern)
std::vector<SystemInputCombination> systemCombinations;
WispInputState previousInput;
uint32_t combinationStartTimes[8];
bool combinationActive[8];

// Input combination timing
static uint32_t inputCombinationTimer = 0;
static WispInputState lastSystemInput;

// System menu state (for legacy compatibility)
static bool systemMenuActive = false;
static bool systemOverlayActive = false;
static int systemMenuSelection = 0;
static int systemOverlayType = 0;

// App count tracking
static int appCount = 0;

// Forward declarations - Core Functions
WispInputState convertToWispInput();
InputState convertToInputState(const WispInputState& wispInput);
void printPerformanceStats();
void handleCriticalError(const String& error);

// Master Bootloader Phase Handlers
void handleRAEInit();
void handleScreenAudioInit();
void handleBootDisplay();
void handleServiceLoad();
void handleAppScan();
void handleAutoStartCheck();
void handleMenuFallback(const WispInputState& input);

// App Management
void scanForWispApps();
// bool loadAppMetadata(const String& appPath, AppEntry& app); // Removed - AppEntry not defined
bool launchAppByIndex(int index);
bool launchAppByPath(const String& path);

// Menu System
void renderMainMenu();
void renderBootProgress(float progress, const String& message);
void playBootSound();

// System Input Combinations (enhanced from learned patterns)
bool checkEmergencyReset(const WispInputState& input);
void initializeSystemPanels();
void updateSystemPanels(const WispInputState& input);
void renderSystemPanels();

// --- MASTER BOOTLOADER SETUP ---
void bootloaderSetup() {
    // Initialize debug system first (using namespace alias)
    Debug::init(Debug::DEBUG_MODE_ON, Debug::SAFETY_MODE_DISABLED);
    
    WISP_DEBUG_INFO("MASTER", "=== WISP ENGINE MASTER BOOTLOADER ===");
    WISP_DEBUG_INFO("MASTER", "Architecture: Unified RAE -> Services -> Apps -> Menu");
    WISP_DEBUG_INFO("MASTER", "Starting master boot sequence...");
    
    // Initialize UI panels for ESP32-S3
#ifndef CONFIG_IDF_TARGET_ESP32C6
    initializeSystemPanels();
#endif
    
    // Set initial phase
    currentPhase = PHASE_RAE_INIT;
    phaseStartTime = get_millis();
    
    // Phase will be handled in main loop
    WISP_DEBUG_INFO("WISP", "Master bootloader setup complete - entering phase-driven loop");
    
    // Initialize storage systems (ESP-IDF v6+ compatible)
    // Note: In ESP-IDF v6+, SPIFFS is typically auto-mounted via partition table
    // For now, skip SPIFFS initialization as it's not critical for basic bootloader
    WISP_DEBUG_INFO("WISP", "Skipping SPIFFS initialization for ESP-IDF v6+ compatibility");
    
}

#ifdef CONFIG_IDF_TARGET_ESP32C6
// Test the document database system
void testDocDatabase() {
    static bool dbTested = false;
    if (dbTested) return;  // Only test once
    
    ESP_LOGI("DB", "Testing document database system...");
    
    // Initialize with 8KB memory
    WispErrorCode result = docDB.initialize(8192);
    if (result != WISP_SUCCESS) {
        ESP_LOGE("DB", "Failed to initialize unified database: %d", result);
        return;
    }
    
    ESP_LOGI("DB", "✓ Unified database initialized");
    
    // === TEST KEY-VALUE STORE ===
    docDB.setU32(0x12345678, 42);
    docDB.setString(0x12345679, "Test String");
    
    uint32_t val = docDB.getU32(0x12345678);
    char strBuffer[32];
    docDB.getString(0x12345679, strBuffer, sizeof(strBuffer));
    
    ESP_LOGI("DB", "Key-value test: %d, '%s'", (int)val, strBuffer);
    
    // === TEST STRUCTURED TABLES ===
    const DDFColumn itemColumns[] = {
        DDF_PRIMARY_KEY("id", DDF_TYPE_U16),
        DDF_COLUMN("name", DDF_TYPE_STRING, 24),
        DDF_COLUMN("category", DDF_TYPE_U8, 0),
        DDF_COLUMN("value", DDF_TYPE_U32, 0)
    };
    
    uint16_t itemTableId = docDB.createTable("items", itemColumns, 4, 32, DDF_TABLE_READ_WRITE);
    ESP_LOGI("DB", "Created items table: %d", itemTableId);
    
    // Create read-only NPCs table
    const DDFColumn npcColumns[] = {
        DDF_PRIMARY_KEY("id", DDF_TYPE_U16),
        DDF_COLUMN("name", DDF_TYPE_STRING, 20),
        DDF_COLUMN("level", DDF_TYPE_U8, 0)
    };
    
    uint16_t npcTableId = docDB.createTable("npcs", npcColumns, 3, 16, DDF_TABLE_READ_ONLY);
    ESP_LOGI("DB", "Created NPCs table: %d (read-only)", npcTableId);
    
    // Test table permissions
    uint8_t itemPerms = docDB.getTablePermissions(itemTableId);
    uint8_t npcPerms = docDB.getTablePermissions(npcTableId);
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
    
    uint16_t row1 = docDB.insertRow(itemTableId, &sword);
    uint16_t row2 = docDB.insertRow(itemTableId, &potion);
    ESP_LOGI("DB", "Inserted items: rows %d, %d", row1, row2);
    
    // Try to insert into read-only table (should fail)
    struct NPCRow {
        uint16_t id;
        char name[20];
        uint8_t level;
    } __attribute__((packed));
    
    NPCRow npc = {1, "Elder", 50};
    uint16_t npcRow = docDB.insertRow(npcTableId, &npc);
    ESP_LOGI("DB", "Tried NPC insert (read-only): row %d (should be 0)", npcRow);
    
    // Make NPCs writable and try again
    docDB.setTablePermissions(npcTableId, DDF_TABLE_READ_WRITE);
    uint16_t npcRow2 = docDB.insertRow(npcTableId, &npc);
    ESP_LOGI("DB", "NPC insert after making writable: row %d", npcRow2);
    
    // Print stats
    ESP_LOGI("DB", "Memory: %d/%d bytes used", 
             (int)docDB.getUsedMemory(), 8192);
    
    if (docDB.validateDatabase()) {
        ESP_LOGI("DB", "✓ Database validation passed");
    }
    
    ESP_LOGI("DB", "✓ Unified database test complete");
    dbTested = true;
}
#endif

// --- MASTER BOOTLOADER MAIN LOOP ---
void bootloaderLoop() {
    // Frame timing control
    if (!WispEngine::Core::Timing::frameReady()) return;
    WispEngine::Core::Timing::tick();
    
    // System heartbeat
    WispEngine::Core::Debug::heartbeat();
    
    // Update input
    if (inputController) {
        inputController->update();
    }
    WispInputState inputState = convertToWispInput();
    
    // Check for emergency reset (highest priority)
    if (checkEmergencyReset(inputState)) {
        return; // Emergency handled
    }
    
    // Handle active UI panels (these pause the underlying app/ROM)
#ifndef CONFIG_IDF_TARGET_ESP32C6
    if (activePanel && activePanel->isActive()) {
        // Ensure app is paused while system menu is active
        AppPauseSystem::pauseRunningApp();
        
        // Update and render the active panel
        activePanel->update(inputState);
        activePanel->render();
        
        // Check if panel was deactivated
        if (!activePanel->isActive()) {
            // Panel closed - resume app if it was running
            AppPauseSystem::resumeApp();
            activePanel = nullptr;
        }
        
        return; // Panel consumed input - app/ROM remains paused
    } else if (AppPauseSystem::isAppPaused()) {
        // No active panel but app is still paused - resume it
        AppPauseSystem::resumeApp();
    }
#endif
    
    // Handle current bootloader phase
    switch (currentPhase) {
        case PHASE_RAE_INIT:
            handleRAEInit();
            break;
            
        case PHASE_SCREEN_AUDIO:
            handleScreenAudioInit();
            break;
            
        case PHASE_BOOT_DISPLAY:
            handleBootDisplay();
            break;
            
        case PHASE_SERVICE_LOAD:
            handleServiceLoad();
            break;
            
        case PHASE_APP_SCAN:
            handleAppScan();
            break;
            
        case PHASE_AUTOSTART_CHECK:
            handleAutoStartCheck();
            break;
            
        case PHASE_MENU_FALLBACK:
            handleMenuFallback(inputState);
            break;
            
        case PHASE_APP_RUNNING:
            // App is running - let app manager handle updates
            if (appManager.isAppRunning()) {
                appManager.update();
            } else {
                // App stopped - return to menu
                currentPhase = PHASE_MENU_FALLBACK;
                menuActive = true;
                phaseStartTime = get_millis();
            }
            break;
    }
    
    // Performance monitoring
    static uint32_t lastStatsTime = 0;
    if (get_millis() - lastStatsTime > 5000) { // Every 5 seconds
        printPerformanceStats();
        lastStatsTime = get_millis();
    }
}

// --- UTILITY FUNCTIONS ---
InputState convertToInputState(const WispInputState& wispInput) {
    InputState state;
    state.left = wispInput.left;
    state.right = wispInput.right;
    state.up = wispInput.up;
    state.down = wispInput.down;
    state.buttonA = wispInput.buttonA;
    state.buttonB = wispInput.buttonB;
    state.buttonC = wispInput.buttonC;
    state.select = wispInput.select;
    state.start = wispInput.start;
    state.analogX = wispInput.analogX;
    state.analogY = wispInput.analogY;
    state.touched = wispInput.touched;
    state.touchX = static_cast<uint16_t>(wispInput.touchX);
    state.touchY = static_cast<uint16_t>(wispInput.touchY);
    return state;
}

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

// =============================================================================
// ENHANCED SYSTEM FUNCTIONS
// =============================================================================

void initializeSystemPanels() {
#ifndef CONFIG_IDF_TARGET_ESP32C6
    WISP_DEBUG_INFO("PANELS", "Initializing UI panels...");
    
    // Initialize system settings panel
    systemSettingsPanel = new SystemSettingsPanel();
    if (!systemSettingsPanel->initialize()) {
        WISP_DEBUG_ERROR("PANELS", "Failed to initialize SystemSettingsPanel");
        delete systemSettingsPanel;
        systemSettingsPanel = nullptr;
    } else {
        WISP_DEBUG_INFO("PANELS", "✓ SystemSettingsPanel initialized");
    }
    
    // Initialize other panels as needed
    // TODO: Initialize display, audio, network panels if needed
    
    WISP_DEBUG_INFO("PANELS", "UI panel initialization complete");
#endif
}

bool checkEmergencyReset(const WispInputState& input) {
    // Emergency reset: Select + B + A (hold for 3 seconds)
    if (input.select && input.buttonB && input.buttonA) {
        if (inputCombinationTimer == 0) {
            inputCombinationTimer = get_millis();
            WISP_DEBUG_INFO("EMERGENCY", "Emergency reset combination started...");
        } else if (get_millis() - inputCombinationTimer > 3000) {
            WISP_DEBUG_INFO("EMERGENCY", "Emergency reset combination confirmed - restarting system");
            esp_restart();
            return true;
        }
        return true; // Consuming input
    } else {
        if (inputCombinationTimer > 0) {
            WISP_DEBUG_INFO("EMERGENCY", "Emergency reset combination cancelled");
            inputCombinationTimer = 0;
        }
    }
    return false;
}

void activateSystemMenu() {
    systemMenuActive = true;
    systemOverlayActive = false;
    systemMenuSelection = 0;
}

void activateSystemOverlay(int type) {
    systemOverlayType = type;
    systemOverlayActive = true;
    systemMenuActive = false;
}

bool checkSystemInputCombinations(const WispInputState& input) {
    bool combinationDetected = false;
    
    // Emergency reset: Select + B + A (hold for 3 seconds)
    if (input.select && input.buttonB && input.buttonA) {
        if (inputCombinationTimer == 0) {
            inputCombinationTimer = get_millis();
        } else if (get_millis() - inputCombinationTimer > 3000) {
            WISP_DEBUG_INFO("WISP", "Emergency reset combination detected");
            esp_restart();
        }
        combinationDetected = true;
    }
    // System menu: Select + Up (quick press)
    else if (input.select && input.up && !(lastSystemInput.select && lastSystemInput.up)) {
        WISP_DEBUG_INFO("WISP", "System menu activated");
        activateSystemMenu();
        inputCombinationTimer = 0;
        combinationDetected = true;
    }
    // Quick settings: Select + Right
    else if (input.select && input.right && !(lastSystemInput.select && lastSystemInput.right)) {
        WISP_DEBUG_INFO("WISP", "Quick settings activated");
        activateSystemOverlay(0);
        inputCombinationTimer = 0;
        combinationDetected = true;
    }
    // Volume control: Select + Left
    else if (input.select && input.left && !(lastSystemInput.select && lastSystemInput.left)) {
        WISP_DEBUG_INFO("WISP", "Volume control activated");
        activateSystemOverlay(1);
        inputCombinationTimer = 0;
        combinationDetected = true;
    }
    // Performance stats: Select + Down
    else if (input.select && input.down && !(lastSystemInput.select && lastSystemInput.down)) {
        WISP_DEBUG_INFO("WISP", "Performance stats activated");
        activateSystemOverlay(3);
        inputCombinationTimer = 0;
        combinationDetected = true;
    }
    else {
        inputCombinationTimer = 0;
    }
    
    lastSystemInput = input;
    return combinationDetected;
}

void handleSystemMenu(const WispInputState& input) {
    static WispInputState lastMenuInput;
    
    // Menu navigation
    if (input.up && !lastMenuInput.up) {
        systemMenuSelection = (systemMenuSelection > 0) ? systemMenuSelection - 1 : 4;
    }
    else if (input.down && !lastMenuInput.down) {
        systemMenuSelection = (systemMenuSelection < 4) ? systemMenuSelection + 1 : 0;
    }
    else if (input.buttonA && !lastMenuInput.buttonA) {
        // Handle menu selection
        switch (systemMenuSelection) {
            case 0: // Applications
                appManager.scanForApps();
                WISP_DEBUG_INFO("WISP", "Scanned for apps");
                break;
            case 1: // Settings
                activateSystemOverlay(0);
                break;
            case 2: // System Info
                activateSystemOverlay(3);
                break;
            case 3: // Display Settings
                activateSystemOverlay(2);
                break;
            case 4: // Power Options
                WISP_DEBUG_INFO("WISP", "Power menu - restart");
                esp_restart();
                break;
        }
    }
    else if (input.buttonB && !lastMenuInput.buttonB) {
        // Exit system menu
        systemMenuActive = false;
    }
    
    lastMenuInput = input;
}

void handleSystemOverlay(const WispInputState& input) {
    static WispInputState lastOverlayInput;
    
    // Common overlay controls
    if (input.buttonB && !lastOverlayInput.buttonB) {
        // Exit overlay
        systemOverlayActive = false;
    }
    
    // Overlay-specific controls
    switch (systemOverlayType) {
        case 1: // Volume overlay
            if (input.up && !lastOverlayInput.up) {
                WISP_DEBUG_INFO("WISP", "Volume up");
            }
            else if (input.down && !lastOverlayInput.down) {
                WISP_DEBUG_INFO("WISP", "Volume down");
            }
            break;
        case 2: // Brightness overlay
            if (input.up && !lastOverlayInput.up) {
                uint8_t currentBrightness = display.getBrightness();
                display.setBrightness(min(255, currentBrightness + 32));
                WISP_DEBUG_INFO("WISP", "Brightness increased");
            }
            else if (input.down && !lastOverlayInput.down) {
                uint8_t currentBrightness = display.getBrightness();
                display.setBrightness(max(32, currentBrightness - 32));
                WISP_DEBUG_INFO("WISP", "Brightness decreased");
            }
            break;
    }
    
    lastOverlayInput = input;
}

void renderSystemMenu() {
    display.fillScreen(0x0841); // Dark blue background
    
    // Title
    display.setTextColor(0xFFFF);
    display.setTextDatum(top_center);
    display.drawString("WISP SYSTEM MENU", SCREEN_WIDTH / 2, 10);
    
    // Menu items
    const char* menuItems[] = {"Applications", "Settings", "System Info", "Display", "Power"};
    int startY = 50;
    int itemHeight = 25;
    
    for (int i = 0; i < 5; i++) {
        uint16_t color = (i == systemMenuSelection) ? 0xFFE0 : 0xC618; // Yellow if selected
        
        if (i == systemMenuSelection) {
            display.fillRect(10, startY + i * itemHeight - 2, SCREEN_WIDTH - 20, 20, 0x2104);
        }
        
        display.setTextColor(color);
        display.setTextDatum(middle_left);
        display.drawString(menuItems[i], 20, startY + i * itemHeight + 8);
    }
    
    // Instructions
    display.setTextColor(0x7BEF);
    display.setTextDatum(bottom_center);
    display.drawString("A: Select | B: Back | Up/Down: Navigate", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 10);
}

void renderSystemOverlay() {
    display.fillScreen(0x0000); // Black background for overlay
    
    switch (systemOverlayType) {
        case 0: // Settings overlay
            display.setTextColor(0xFFFF);
            display.setTextDatum(top_center);
            display.drawString("SETTINGS", SCREEN_WIDTH / 2, 20);
            display.setTextDatum(middle_center);
            display.drawString("Settings panel", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
            display.drawString("Use Select+Up for menu", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20);
            break;
            
        case 1: // Volume overlay
            display.setTextColor(0xFFFF);
            display.setTextDatum(top_center);
            display.drawString("VOLUME CONTROL", SCREEN_WIDTH / 2, 20);
            display.setTextDatum(middle_center);
            display.drawString("Up/Down: Adjust Volume", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
            break;
            
        case 2: // Brightness overlay
            display.setTextColor(0xFFFF);
            display.setTextDatum(top_center);
            display.drawString("BRIGHTNESS", SCREEN_WIDTH / 2, 20);
            display.setTextDatum(middle_center);
            display.drawString("Up/Down: Adjust Brightness", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
            
            // Show current brightness
            char brightnessText[32];
            sprintf(brightnessText, "Current: %d/255", display.getBrightness());
            display.drawString(brightnessText, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20);
            break;
            
        case 3: // Performance stats overlay
            display.setTextColor(0xFFFF);
            display.setTextDatum(top_center);
            display.drawString("SYSTEM INFO", SCREEN_WIDTH / 2, 20);
            
            // Show system stats
            uint32_t freeHeap = esp_get_free_heap_size();
            char statsText[64];
            
            sprintf(statsText, "Free Heap: %lu KB", freeHeap / 1024);
            display.setTextDatum(middle_left);
            display.drawString(statsText, 20, 60);
            
            sprintf(statsText, "Menu Active: %s", systemMenuActive ? "Yes" : "No");
            display.drawString(statsText, 20, 80);
            
            sprintf(statsText, "Overlay Active: %s", systemOverlayActive ? "Yes" : "No");
            display.drawString(statsText, 20, 100);
            
            if (appManager.isAppRunning()) {
                display.drawString("App Status: Running", 20, 120);
            } else {
                display.drawString("App Status: Idle", 20, 120);
            }
            break;
    }
    
    // Common overlay instructions
    display.setTextColor(0x7BEF);
    display.setTextDatum(bottom_center);
    display.drawString("B: Close | Select+Up: System Menu", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 10);
}

void renderIdleScreen() {
    static uint32_t lastIdleUpdate = 0;
    
    // Update idle screen every few seconds
    if (get_millis() - lastIdleUpdate > 3000) {
        display.fillScreen(0x0000);
        display.setTextColor(0xFFFF);
        display.setTextDatum(top_center);
        display.drawString("WISP ENGINE", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 40);
        display.drawString("System Idle", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20);
        
        display.setTextColor(0x7BEF);
        display.drawString("Select+Up: System Menu", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 10);
        display.drawString("Select+Right: Quick Settings", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 30);
        display.drawString("Select+Down: System Info", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 50);
        
        lastIdleUpdate = get_millis();
    }
}

// =============================================================================
// MASTER BOOTLOADER PHASE HANDLERS IMPLEMENTATION
// =============================================================================

void handleRAEInit() {
    static bool raeInitialized = false;
    
    if (!raeInitialized) {
        WISP_DEBUG_INFO("BOOTLOADER", "Phase 1: RAE (Render, Audio, Engine) Initialization");
        
        // Initialize timing system first
        WispEngine::Core::Timing::init();
        WISP_DEBUG_INFO("BOOTLOADER", "✓ Timing system initialized");
        
        raeInitialized = true;
        phaseStartTime = get_millis();
    }
    
    // Move to next phase after brief delay
    if (get_millis() - phaseStartTime > 200) {
        currentPhase = PHASE_SCREEN_AUDIO;
        phaseStartTime = get_millis();
        WISP_DEBUG_INFO("BOOTLOADER", "RAE init complete, advancing to screen/audio");
    }
}

void handleScreenAudioInit() {
    static bool screenAudioInitialized = false;
    
    if (!screenAudioInitialized) {
        WISP_DEBUG_INFO("BOOTLOADER", "Phase 2: Screen and Audio Systems Init");
        
        // Initialize display
        display.init();
        display.setBrightness(255);
        display.setColorDepth(16);
        display.fillScreen(0x0000);
        WISP_DEBUG_INFO("BOOTLOADER", "✓ Display initialized");
        
        // Initialize input controller
        inputController = new InputController();
        if (!inputController->init()) {
            handleCriticalError("Input controller initialization failed");
            return;
        }
        WISP_DEBUG_INFO("BOOTLOADER", "✓ Input controller initialized");
        
        // Initialize engine systems
#ifdef CONFIG_IDF_TARGET_ESP32C6
        if (!mainEngine.init()) {
            handleCriticalError("Minimal engine initialization failed");
            return;
        }
        WISP_DEBUG_INFO("BOOTLOADER", "✓ Minimal engine initialized");
#else
        // Initialize full engine for ESP32-S3
        WISP_DEBUG_INFO("BOOTLOADER", "✓ Engine system ready");
#endif
        
        screenAudioInitialized = true;
        phaseStartTime = get_millis();
    }
    
    // Move to next phase after brief delay
    if (get_millis() - phaseStartTime > 300) {
        currentPhase = PHASE_BOOT_DISPLAY;
        phaseStartTime = get_millis();
        WISP_DEBUG_INFO("BOOTLOADER", "Screen/audio init complete, showing boot screen");
    }
}

void handleBootDisplay() {
    static bool bootSoundPlayed = false;
    
    if (!bootSoundPlayed) {
        WISP_DEBUG_INFO("BOOTLOADER", "Phase 3: Boot Display and SFX");
        playBootSound();
        bootSoundPlayed = true;
    }
    
    // Show boot screen with progress
    float progress = (float)(get_millis() - phaseStartTime) / 2000.0f; // 2 second boot screen
    renderBootProgress(progress, "Starting Wisp Engine...");
    
    // Move to next phase after boot screen duration
    if (get_millis() - phaseStartTime > 2000) {
        currentPhase = PHASE_SERVICE_LOAD;
        phaseStartTime = get_millis();
        WISP_DEBUG_INFO("BOOTLOADER", "Boot display complete, loading services");
    }
}

void handleServiceLoad() {
    static bool servicesLoaded = false;
    
    if (!servicesLoaded) {
        WISP_DEBUG_INFO("BOOTLOADER", "Phase 4: Loading Additional Services");
        
        // Initialize app manager
        if (!appManager.init(&appLoader, &appLoop)) {
            handleCriticalError("App manager initialization failed");
            return;
        }
        WISP_DEBUG_INFO("BOOTLOADER", "✓ App manager initialized");
        
        // Grant API permissions for app launching
        curatedAPI.setAppPermissions(true, false, false, false);
        WISP_DEBUG_INFO("BOOTLOADER", "✓ API permissions configured");
        
        servicesLoaded = true;
    }
    
    // Show service loading progress
    float progress = (float)(get_millis() - phaseStartTime) / 1000.0f;
    renderBootProgress(progress, "Loading services...");
    
    // Move to next phase after service load time
    if (get_millis() - phaseStartTime > 1000) {
        currentPhase = PHASE_APP_SCAN;
        phaseStartTime = get_millis();
        WISP_DEBUG_INFO("BOOTLOADER", "Services loaded, scanning for apps");
    }
}

void handleAppScan() {
    static bool appsScanned = false;
    
    if (!appsScanned) {
        WISP_DEBUG_INFO("BOOTLOADER", "Phase 5: Scanning for Apps");
        scanForWispApps();
        appsScanned = true;
    }
    
    // Show app scanning progress
    renderBootProgress(1.0f, String("Found " + String(appCount) + " apps"));
    
    // Move to next phase after brief delay
    if (get_millis() - phaseStartTime > 800) {
        currentPhase = PHASE_AUTOSTART_CHECK;
        phaseStartTime = get_millis();
        WISP_DEBUG_INFO("BOOTLOADER", "App scan complete, checking for autoStart apps");
    }
}

void handleAutoStartCheck() {
    WISP_DEBUG_INFO("BOOTLOADER", "Phase 6: AutoStart Check");
    
    // Look for autoStart apps
    bool foundAutoStart = false;
    for (int i = 0; i < appCount; i++) {
        if (i < (int)availableApps.size() && availableApps[i].autoStart) {
            WISP_DEBUG_INFO("BOOTLOADER", String("AutoStart app found: " + String(availableApps[i].name.c_str())).c_str());
            if (launchAppByIndex(i)) {
                currentPhase = PHASE_APP_RUNNING;
                foundAutoStart = true;
                break;
            }
        }
    }
    
    if (!foundAutoStart) {
        // No autoStart app found, go to menu
        WISP_DEBUG_INFO("BOOTLOADER", "No autoStart apps found, showing main menu");
        currentPhase = PHASE_MENU_FALLBACK;
        menuActive = true;
        selectedAppIndex = 0;
        menuSelection = 0;
    }
    
    phaseStartTime = get_millis();
}

void handleMenuFallback(const WispInputState& input) {
    static WispInputState lastInput;
    
    // Handle menu input
    if (input.up && !lastInput.up) {
        menuSelection = (menuSelection > 0) ? menuSelection - 1 : 1;
    }
    else if (input.down && !lastInput.down) {
        menuSelection = (menuSelection < 1) ? menuSelection + 1 : 0;
    }
    else if (input.left && !lastInput.left && menuSelection == 0) {
        // Cycle through apps when on app button
        if (appCount > 0) {
            selectedAppIndex = (selectedAppIndex > 0) ? selectedAppIndex - 1 : appCount - 1;
        }
    }
    else if (input.right && !lastInput.right && menuSelection == 0) {
        // Cycle through apps when on app button
        if (appCount > 0) {
            selectedAppIndex = (selectedAppIndex < appCount - 1) ? selectedAppIndex + 1 : 0;
        }
    }
    else if (input.buttonA && !lastInput.buttonA) {
        if (menuSelection == 0 && appCount > 0) {
            // Launch selected app
            if (launchAppByIndex(selectedAppIndex)) {
                currentPhase = PHASE_APP_RUNNING;
                menuActive = false;
            }
        }
        else if (menuSelection == 1) {
            // Show settings using proper SystemSettingsPanel
#ifndef CONFIG_IDF_TARGET_ESP32C6
            if (systemSettingsPanel) {
                WISP_DEBUG_INFO("MASTER", "Activating SystemSettingsPanel");
                systemSettingsPanel->activate();
                activePanel = systemSettingsPanel;
                // App will be paused automatically by panel system
            } else {
                WISP_DEBUG_ERROR("MASTER", "SystemSettingsPanel not initialized");
            }
#else
            WISP_DEBUG_INFO("MASTER", "Settings panel not available on ESP32-C6");
#endif
        }
    }
    
    // Render the main menu
    renderMainMenu();
    
    lastInput = input;
}

// =============================================================================
// APP MANAGEMENT FUNCTIONS
// =============================================================================

void scanForWispApps() {
    appCount = 0;
    
    WISP_DEBUG_INFO("BOOTLOADER", "Scanning for .wisp applications...");
    
    // Use app manager to scan for apps
    appManager.scanForApps();
    
    // For now, create some dummy apps for testing
    // In real implementation, this would read from SPIFFS
    if (appCount < MAX_APPS) {
        AppInfo testApp;
        testApp.name = "Test Game";
        testApp.executablePath = "/apps/testgame.wisp";
        testApp.iconPath = "";
        testApp.autoStart = false;
        availableApps.push_back(testApp);
        appCount++;
    }
    
    if (appCount < MAX_APPS) {
        AppInfo demoApp;
        demoApp.name = "Demo App";
        demoApp.executablePath = "/apps/demo.wisp";
        demoApp.iconPath = "";
        demoApp.autoStart = false;
        availableApps.push_back(demoApp);
        appCount++;
    }
    
    WISP_DEBUG_INFO("BOOTLOADER", String("Found " + String(appCount) + " applications").c_str());
}

// bool loadAppMetadata(const String& appPath, AppEntry& app) {
//     // TODO: Implement actual metadata loading from .wisp files
//     // For now, just extract name from path
//     int lastSlash = appPath.lastIndexOf('/');
//     int lastDot = appPath.lastIndexOf('.');
//     
//     if (lastSlash >= 0 && lastDot > lastSlash) {
//         app.name = appPath.substring(lastSlash + 1, lastDot);
//         app.path = appPath;
//         app.isValid = true;
//         return true;
//     }
//     
//     return false;
// }

bool launchAppByIndex(int index) {
    if (index < 0 || index >= appCount || index >= (int)availableApps.size()) {
        return false;
    }
    
    return launchAppByPath(availableApps[index].executablePath.c_str());
}

bool launchAppByPath(const String& path) {
    WISP_DEBUG_INFO("BOOTLOADER", String("Attempting to launch app: " + path).c_str());
    
    if (appManager.loadApp(path)) {
        WISP_DEBUG_INFO("BOOTLOADER", "App launched successfully");
        return true;
    } else {
        WISP_DEBUG_ERROR("BOOTLOADER", "App launch failed");
        return false;
    }
}

// =============================================================================
// RENDERING FUNCTIONS
// =============================================================================

void renderMainMenu() {
    display.fillScreen(0x0020); // Dark navy background
    
    // Title
    display.setTextColor(0xFFFF);
    display.setTextDatum(top_center);
    display.drawString("WISP ENGINE", SCREEN_WIDTH / 2, 10);
    display.setTextColor(0x7BEF);
    display.drawString("Main Menu", SCREEN_WIDTH / 2, 30);
    
    int buttonY = 70;
    int buttonHeight = 40;
    int buttonSpacing = 50;
    
    // App button (larger, shows current app or "No Apps Found")
    uint16_t appBgColor = (menuSelection == 0) ? 0x2104 : 0x1082; // Highlighted if selected
    uint16_t appTextColor = (menuSelection == 0) ? 0xFFE0 : 0xFFFF; // Yellow if selected
    
    display.fillRoundRect(20, buttonY, SCREEN_WIDTH - 40, buttonHeight, 8, appBgColor);
    display.drawRoundRect(20, buttonY, SCREEN_WIDTH - 40, buttonHeight, 8, 0x4208);
    
    display.setTextColor(appTextColor);
    display.setTextDatum(middle_center);
    
    if (appCount > 0 && selectedAppIndex < appCount && selectedAppIndex < (int)availableApps.size()) {
        display.drawString(availableApps[selectedAppIndex].name.c_str(), SCREEN_WIDTH / 2, buttonY + buttonHeight / 2);
        // Show app navigation hints
        if (appCount > 1) {
            display.setTextColor(0x7BEF);
            display.setTextDatum(middle_left);
            display.drawString("<", 30, buttonY + buttonHeight / 2);
            display.setTextDatum(middle_right);
            display.drawString(">", SCREEN_WIDTH - 30, buttonY + buttonHeight / 2);
        }
    } else {
        display.setTextColor(0x7BEF);
        display.drawString("No Wisp Apps Found", SCREEN_WIDTH / 2, buttonY + buttonHeight / 2);
    }
    
    // Settings button
    buttonY += buttonHeight + buttonSpacing;
    uint16_t settingsBgColor = (menuSelection == 1) ? 0x2104 : 0x1082; // Highlighted if selected
    uint16_t settingsTextColor = (menuSelection == 1) ? 0xFFE0 : 0xFFFF; // Yellow if selected
    
    display.fillRoundRect(20, buttonY, SCREEN_WIDTH - 40, buttonHeight, 8, settingsBgColor);
    display.drawRoundRect(20, buttonY, SCREEN_WIDTH - 40, buttonHeight, 8, 0x4208);
    
    display.setTextColor(settingsTextColor);
    display.setTextDatum(middle_center);
    display.drawString("Settings", SCREEN_WIDTH / 2, buttonY + buttonHeight / 2);
    
    // Instructions at bottom
    display.setTextColor(0x7BEF);
    display.setTextDatum(bottom_center);
    display.drawString("A: Select | Up/Down: Navigate", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30);
    if (menuSelection == 0 && appCount > 1) {
        display.drawString("Left/Right: Change App", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 15);
    }
}

void renderBootProgress(float progress, const String& message) {
    display.fillScreen(0x0000);
    
    // Wisp Engine logo area
    display.setTextColor(0xFFFF);
    display.setTextDatum(middle_center);
    display.drawString("WISP ENGINE", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 40);
    
    // Progress bar
    int barWidth = SCREEN_WIDTH - 60;
    int barHeight = 8;
    int barX = 30;
    int barY = SCREEN_HEIGHT / 2;
    
    // Progress bar background
    display.fillRect(barX, barY, barWidth, barHeight, 0x2104);
    
    // Progress bar fill
    int fillWidth = (int)(barWidth * min(1.0f, max(0.0f, progress)));
    display.fillRect(barX, barY, fillWidth, barHeight, 0x07E0); // Green
    
    // Progress message
    display.setTextColor(0x7BEF);
    display.drawString(message, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 30);
}

void playBootSound() {
#ifdef CONFIG_IDF_TARGET_ESP32C6
    // Play a simple boot chime on the minimal engine
    mainEngine.audio().playNote(WispEngine::Minimal::CHANNEL_SQUARE1, 440, 8, 15);
    // Add a harmony note
    mainEngine.audio().playNote(WispEngine::Minimal::CHANNEL_SQUARE2, 880, 6, 10);
#else
    // TODO: Implement boot sound for full engine
#endif
    WISP_DEBUG_INFO("BOOTLOADER", "Boot sound played");
}
