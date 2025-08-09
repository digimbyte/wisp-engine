// enhanced_bootloader.h - System-Level Bootloader with Idle Mode Support
#pragma once

#include "esp32_common.h"
#include <LovyanGFX.hpp>
#include "../engine/app/curated_api.h"
#include "lazy_resource_manager.h"
#include "app_loop_manager.h"
#include "graphics_engine.h" 
#include "wisp_sprite_layers.h"
#include "native_app_loader.h"
#include "definitions.h"
#include "settings_manager.h"
#include "ui/panels/menu.h"
#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <vector>

static const char* BOOTLOADER_TAG = "WispSystemBootloader";

// Enhanced boot sequence states
enum BootStage {
    BOOT_HARDWARE_INIT,
    BOOT_ENGINE_INIT,
    BOOT_SYSTEM_SERVICES,    // Initialize system services
    BOOT_RECOVERY_CHECK,     // Check for recovery mode
    BOOT_SPLASH_DISPLAY,
    BOOT_APP_DISCOVERY,
    BOOT_MODE_DECISION,      // Decide boot mode (menu/auto-launch)
    BOOT_MENU_IDLE,          // Idle in system menu
    BOOT_APP_PREPARE,
    BOOT_APP_LAUNCH,
    BOOT_APP_RUNNING,        // App running with system monitoring
    BOOT_SYSTEM_OVERLAY      // System overlay active
};

// System overlay modes
enum SystemOverlayMode {
    OVERLAY_NONE = 0,
    OVERLAY_QUICK_SETTINGS,     // Quick settings strip
    OVERLAY_VOLUME_CONTROL,     // Volume adjustment
    OVERLAY_BRIGHTNESS_CONTROL, // Brightness adjustment
    OVERLAY_TASK_SWITCHER,      // App switcher
    OVERLAY_NOTIFICATION_BAR,   // System notifications
    OVERLAY_PERFORMANCE_STATS,  // Debug performance info
    OVERLAY_FULL_MENU          // Full system menu
};

// Input priority levels
enum InputPriority {
    PRIORITY_EMERGENCY = 0,     // Emergency reset, recovery mode
    PRIORITY_SYSTEM = 1,        // System menu, quick settings
    PRIORITY_APP_OVERRIDE = 2,  // App-level system integration
    PRIORITY_APP_NORMAL = 3     // Normal app input
};

// System input combination configuration
struct SystemInputCombination {
    uint32_t buttons;           // Button combination mask
    uint32_t holdTime;          // Hold time in milliseconds
    SystemOverlayMode action;   // Action to trigger
    InputPriority priority;     // Priority level
    bool requiresAllButtons;    // All buttons must be pressed
    const char* description;    // Human-readable description
    
    SystemInputCombination(uint32_t btns, uint32_t hold, SystemOverlayMode act, 
                          InputPriority prio, bool all, const char* desc) :
        buttons(btns), holdTime(hold), action(act), priority(prio), 
        requiresAllButtons(all), description(desc) {}
};

// System state tracking
struct SystemState {
    bool appRunning = false;
    bool systemOverlayActive = false;
    bool emergencyMode = false;
    bool recoveryMode = false;
    bool idleMode = false;
    SystemOverlayMode overlayMode = OVERLAY_NONE;
    AppInfo* currentApp = nullptr;
    uint32_t systemIdleTime = 0;
    uint32_t lastUserInput = 0;
    uint32_t bootTime = 0;
    InputPriority currentInputPriority = PRIORITY_APP_NORMAL;
};

// Input processing result
struct InputProcessResult {
    bool systemHandled = false;
    bool appCanReceive = true;
    SystemOverlayMode triggeredOverlay = OVERLAY_NONE;
    WispInputState filteredInput;
};

// System menu categories
enum SystemMenuCategory {
    MENU_APPLICATIONS = 0,
    MENU_SETTINGS,
    MENU_SYSTEM_INFO,
    MENU_TOOLS,
    MENU_POWER,
    MENU_CATEGORY_COUNT
};

// Enhanced bootloader class with system-level functionality
class WispSystemBootloader {
private:
    // Core systems (inherited from original bootloader)
    LGFX* display;
    LazyResourceManager* resourceManager;
    AppLoopManager* appLoop;
    GraphicsEngine* graphics;
    WispCuratedAPI* api;
    WispSpriteLayerSystem* layerSystem;
    NativeAppLoader* appLoader;
    
    // System services
    using namespace WispEngine::System;
    SettingsManager* settingsManager;
    
    // Boot state
    BootStage currentStage;
    uint32_t stageStartTime;
    uint32_t splashDuration;
    SystemState systemState;
    
    // App management
    std::vector<AppInfo> availableApps;
    int selectedAppIndex;
    AppInfo currentApp;
    
    // System menu state
    SystemMenuCategory currentMenuCategory;
    int menuCategorySelection;
    int menuItemSelection;
    bool inSubMenu;
    MenuPanel* activePanel;
    
    // Input handling
    std::vector<SystemInputCombination> systemCombinations;
    WispInputState previousInput;
    uint32_t combinationStartTimes[8];
    bool combinationActive[8];
    
    // Settings panels (persistent)
    MenuPanel* displaySettingsPanel;
    MenuPanel* audioSettingsPanel;
    MenuPanel* networkSettingsPanel;
    MenuPanel* systemSettingsPanel;
    
    // Screen resolution
    uint16_t deviceScreenWidth;
    uint16_t deviceScreenHeight;

public:
    WispSystemBootloader(LGFX* disp, LazyResourceManager* resMgr, AppLoopManager* loop,
                        GraphicsEngine* gfx, WispCuratedAPI* apiPtr, NativeAppLoader* loader) :
        display(disp), resourceManager(resMgr), appLoop(loop), graphics(gfx), api(apiPtr),
        layerSystem(nullptr), appLoader(loader), currentStage(BOOT_HARDWARE_INIT),
        stageStartTime(0), splashDuration(2000), selectedAppIndex(-1),
        currentMenuCategory(MENU_APPLICATIONS), menuCategorySelection(0),
        menuItemSelection(0), inSubMenu(false), activePanel(nullptr),
        displaySettingsPanel(nullptr), audioSettingsPanel(nullptr),
        networkSettingsPanel(nullptr), systemSettingsPanel(nullptr) {
        
        deviceScreenWidth = SCREEN_WIDTH;
        deviceScreenHeight = SCREEN_HEIGHT;
        systemState.bootTime = get_millis();
        
        // Initialize system input combinations
        initializeSystemCombinations();
        
        // Clear combination tracking
        for (int i = 0; i < 8; i++) {
            combinationStartTimes[i] = 0;
            combinationActive[i] = false;
        }
    }
    
    // Core bootloader interface
    bool init();
    void update();
    void render();
    
    // Enhanced system functionality
    bool canIdleWithoutApp() const { return true; }
    void enterSystemMenu();
    void exitSystemMenu();
    void showSystemOverlay(SystemOverlayMode mode);
    void hideSystemOverlay();
    
    // App lifecycle management
    bool launchApp(const AppInfo& app);
    bool launchApp(int appIndex);
    bool terminateApp();
    bool suspendApp();
    bool resumeApp();
    
    // System input handling
    InputProcessResult processInput(const WispInputState& rawInput);
    bool checkSystemOverride(const WispInputState& input);
    void handleSystemInput(const WispInputState& input);
    
    // System state queries
    BootStage getCurrentStage() const { return currentStage; }
    bool isBootComplete() const { return currentStage == BOOT_APP_RUNNING || currentStage == BOOT_MENU_IDLE; }
    bool isAppRunning() const { return systemState.appRunning; }
    bool isSystemOverlayActive() const { return systemState.systemOverlayActive; }
    bool isInIdleMode() const { return systemState.idleMode; }
    SystemOverlayMode getOverlayMode() const { return systemState.overlayMode; }
    
    // App management
    void scanForApps();
    size_t getAppCount() const { return availableApps.size(); }
    const AppInfo* getApp(int index) const;
    const std::vector<AppInfo>& getApps() const { return availableApps; }

private:
    // Boot stage handlers
    void handleHardwareInit();
    void handleEngineInit();
    void handleSystemServicesInit();
    void handleRecoveryCheck();
    void handleSplashDisplay();
    void handleAppDiscovery();
    void handleModeDecision();
    void handleMenuIdle();
    void handleAppPrepare();
    void handleAppLaunch();
    void handleAppRunning();
    void handleSystemOverlay();
    
    // System initialization
    void initializeSystemServices();
    void initializeSystemCombinations();
    void initializeSystemPanels();
    
    // System menu management
    void updateSystemMenu(const WispInputState& input);
    void renderSystemMenu();
    void renderMenuCategory(SystemMenuCategory category);
    void navigateToCategory(SystemMenuCategory category);
    void activateMenuItem();
    
    // System overlay management
    void updateSystemOverlay(const WispInputState& input);
    void renderSystemOverlay();
    void renderQuickSettings();
    void renderVolumeControl();
    void renderBrightnessControl();
    void renderPerformanceStats();
    
    // Input combination processing
    bool checkInputCombination(const SystemInputCombination& combo, const WispInputState& input);
    void updateCombinationTracking(const WispInputState& input);
    uint32_t getInputMask(const WispInputState& input) const;
    
    // Rendering helpers
    void renderWispSplash();
    void renderBootProgress(const std::string& message, float progress);
    void renderIdleScreen();
    void centerText(const std::string& text, int y, uint16_t color = 0xFFFF);
    void drawMenuItem(const std::string& text, int index, int y, bool selected, bool enabled = true);
    void drawSystemStatus();
    
    // App loading helpers
    bool loadAppInfo(const std::string& appPath, AppInfo& info);
    bool parseAppManifest(const std::string& manifestPath, AppInfo& info);
    bool validateAppBinary(const std::string& binaryPath);
    
    // System state management
    void updateSystemState();
    void updateIdleTimeout();
    void checkSystemHealth();
    void handleEmergencyMode();
    
    // Resource management
    void reserveSystemResources();
    void freeAppResources();
    void enforceResourceLimits();
    
    // Utility functions
    void advanceStage();
    bool isStageTimeout() const;
    float getStageProgress() const;
    void setInputPriority(InputPriority priority);
    
    // System combinations
    static const uint32_t INPUT_SELECT = 0x01;
    static const uint32_t INPUT_START = 0x02;
    static const uint32_t INPUT_A = 0x04;
    static const uint32_t INPUT_B = 0x08;
    static const uint32_t INPUT_UP = 0x10;
    static const uint32_t INPUT_DOWN = 0x20;
    static const uint32_t INPUT_LEFT = 0x40;
    static const uint32_t INPUT_RIGHT = 0x80;
};

// Implementation of core methods
inline bool WispSystemBootloader::init() {
    ESP_LOGI(BOOTLOADER_TAG, "Initializing Wisp System Bootloader...");
    
    currentStage = BOOT_HARDWARE_INIT;
    stageStartTime = get_millis();
    splashDuration = 2000;
    
    // Initialize system state
    systemState.bootTime = get_millis();
    systemState.idleMode = false;
    systemState.currentInputPriority = PRIORITY_APP_NORMAL;
    
    // Get settings manager instance
    settingsManager = &SettingsManager::getInstance();
    
    ESP_LOGI(BOOTLOADER_TAG, "System Bootloader initialized");
    return true;
}

inline void WispSystemBootloader::initializeSystemCombinations() {
    systemCombinations.clear();
    
    // Primary system access (always available)
    systemCombinations.emplace_back(
        INPUT_SELECT | INPUT_START, 2000, OVERLAY_FULL_MENU, 
        PRIORITY_SYSTEM, true, "Enter System Menu"
    );
    
    // Emergency reset
    systemCombinations.emplace_back(
        INPUT_A | INPUT_B | INPUT_SELECT | INPUT_START, 5000, OVERLAY_NONE,
        PRIORITY_EMERGENCY, true, "Emergency Reset"
    );
    
    // Quick access shortcuts
    systemCombinations.emplace_back(
        INPUT_SELECT | INPUT_UP, 1000, OVERLAY_QUICK_SETTINGS,
        PRIORITY_SYSTEM, true, "Quick Settings"
    );
    
    systemCombinations.emplace_back(
        INPUT_SELECT | INPUT_DOWN, 1000, OVERLAY_TASK_SWITCHER,
        PRIORITY_SYSTEM, true, "Task Switcher"
    );
    
    // Volume/brightness control
    systemCombinations.emplace_back(
        INPUT_SELECT | INPUT_LEFT, 0, OVERLAY_VOLUME_CONTROL,
        PRIORITY_SYSTEM, false, "Volume Down"
    );
    
    systemCombinations.emplace_back(
        INPUT_SELECT | INPUT_RIGHT, 0, OVERLAY_VOLUME_CONTROL,
        PRIORITY_SYSTEM, false, "Volume Up"
    );
    
    systemCombinations.emplace_back(
        INPUT_START | INPUT_LEFT, 0, OVERLAY_BRIGHTNESS_CONTROL,
        PRIORITY_SYSTEM, false, "Brightness Down"
    );
    
    systemCombinations.emplace_back(
        INPUT_START | INPUT_RIGHT, 0, OVERLAY_BRIGHTNESS_CONTROL,
        PRIORITY_SYSTEM, false, "Brightness Up"
    );
    
    // Debug shortcuts
    systemCombinations.emplace_back(
        INPUT_START | INPUT_SELECT | INPUT_B, 0, OVERLAY_PERFORMANCE_STATS,
        PRIORITY_SYSTEM, true, "Performance Stats"
    );
    
    ESP_LOGI(BOOTLOADER_TAG, "Initialized %zu system input combinations", systemCombinations.size());
}

inline void WispSystemBootloader::update() {
    // Update system state
    updateSystemState();
    
    // Process boot stages
    switch (currentStage) {
        case BOOT_HARDWARE_INIT:
            handleHardwareInit();
            break;
        case BOOT_ENGINE_INIT:
            handleEngineInit();
            break;
        case BOOT_SYSTEM_SERVICES:
            handleSystemServicesInit();
            break;
        case BOOT_RECOVERY_CHECK:
            handleRecoveryCheck();
            break;
        case BOOT_SPLASH_DISPLAY:
            handleSplashDisplay();
            break;
        case BOOT_APP_DISCOVERY:
            handleAppDiscovery();
            break;
        case BOOT_MODE_DECISION:
            handleModeDecision();
            break;
        case BOOT_MENU_IDLE:
            handleMenuIdle();
            break;
        case BOOT_APP_PREPARE:
            handleAppPrepare();
            break;
        case BOOT_APP_LAUNCH:
            handleAppLaunch();
            break;
        case BOOT_APP_RUNNING:
            handleAppRunning();
            break;
        case BOOT_SYSTEM_OVERLAY:
            handleSystemOverlay();
            break;
    }
}

inline void WispSystemBootloader::handleSystemServicesInit() {
    ESP_LOGI(BOOTLOADER_TAG, "Initializing system services...");
    
    // Initialize system panels
    initializeSystemPanels();
    
    // Reserve system resources
    reserveSystemResources();
    
    ESP_LOGI(BOOTLOADER_TAG, "System services initialized");
    advanceStage();
}

inline void WispSystemBootloader::handleModeDecision() {
    ESP_LOGI(BOOTLOADER_TAG, "Deciding boot mode...");
    
    // Check for auto-launch app
    bool hasAutoLaunchApp = false;
    for (const auto& app : availableApps) {
        if (app.autoStart) {
            selectedAppIndex = &app - &availableApps[0];
            hasAutoLaunchApp = true;
            break;
        }
    }
    
    // Decision logic
    if (systemState.recoveryMode) {
        ESP_LOGI(BOOTLOADER_TAG, "Entering recovery mode");
        currentStage = BOOT_MENU_IDLE;
        systemState.idleMode = true;
    } else if (hasAutoLaunchApp && !systemState.emergencyMode) {
        ESP_LOGI(BOOTLOADER_TAG, "Auto-launching application");
        currentStage = BOOT_APP_PREPARE;
    } else {
        ESP_LOGI(BOOTLOADER_TAG, "Entering system menu idle mode");
        currentStage = BOOT_MENU_IDLE;
        systemState.idleMode = true;
    }
    
    stageStartTime = get_millis();
}

inline void WispSystemBootloader::handleMenuIdle() {
    // System is idling in menu - this is the persistent state
    systemState.idleMode = true;
    systemState.appRunning = false;
    
    // Update idle timeout
    updateIdleTimeout();
    
    // Check system health
    checkSystemHealth();
    
    // Menu remains active and responsive
}

inline void WispSystemBootloader::enterSystemMenu() {
    ESP_LOGI(BOOTLOADER_TAG, "Entering system menu");
    
    if (systemState.appRunning) {
        // Suspend current app
        suspendApp();
    }
    
    currentStage = BOOT_SYSTEM_OVERLAY;
    systemState.systemOverlayActive = true;
    systemState.overlayMode = OVERLAY_FULL_MENU;
    currentMenuCategory = MENU_APPLICATIONS;
    menuCategorySelection = 0;
    menuItemSelection = 0;
    inSubMenu = false;
    
    // Set system input priority
    setInputPriority(PRIORITY_SYSTEM);
}

inline bool WispSystemBootloader::checkSystemOverride(const WispInputState& input) {
    // Update combination tracking
    updateCombinationTracking(input);
    
    // Check each system combination
    for (size_t i = 0; i < systemCombinations.size(); i++) {
        if (checkInputCombination(systemCombinations[i], input)) {
            const auto& combo = systemCombinations[i];
            
            ESP_LOGI(BOOTLOADER_TAG, "System combination triggered: %s", combo.description);
            
            // Handle special cases
            if (combo.action == OVERLAY_NONE && combo.priority == PRIORITY_EMERGENCY) {
                // Emergency reset
                ESP_LOGW(BOOTLOADER_TAG, "Emergency reset requested!");
                esp_restart();
                return true;
            }
            
            // Show overlay
            if (combo.action != OVERLAY_NONE) {
                showSystemOverlay(combo.action);
                return true;
            }
        }
    }
    
    return false;
}

inline InputProcessResult WispSystemBootloader::processInput(const WispInputState& rawInput) {
    InputProcessResult result;
    result.filteredInput = rawInput;
    
    // Check for system overrides first
    if (checkSystemOverride(rawInput)) {
        result.systemHandled = true;
        result.appCanReceive = false;
        return result;
    }
    
    // Allow app to receive input if no system override
    result.appCanReceive = (systemState.currentInputPriority >= PRIORITY_APP_NORMAL);
    
    return result;
}

inline void WispSystemBootloader::scanForApps() {
    availableApps.clear();
    
    ESP_LOGI(BOOTLOADER_TAG, "Scanning for applications...");
    
    // Implementation similar to original bootloader but enhanced
    // ... (scan SPIFFS and SD card for .wisp files)
    
    ESP_LOGI(BOOTLOADER_TAG, "Found %zu applications", availableApps.size());
    
    // Set first app as default selection if available
    if (!availableApps.empty()) {
        selectedAppIndex = 0;
    }
}

#endif // ENHANCED_BOOTLOADER_H
