// src/system/wisp_bootloader.h
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
#include <dirent.h>
#include <sys/stat.h>
#include <string>

static const char* TAG = "WispBootloader";

// Boot sequence states
enum BootStage {
    BOOT_HARDWARE_INIT,     // Initialize display, input, audio
    BOOT_ENGINE_INIT,       // Initialize core engine systems
    BOOT_SPLASH_DISPLAY,    // Show Wisp Engine logo
    BOOT_APP_DISCOVERY,     // Scan for available apps
    BOOT_APP_SPLASH,        // Show app logo if exists
    BOOT_MENU_CHECK,        // Check if auto-start or show menu
    BOOT_MENU_DISPLAY,      // Show device menu
    BOOT_APP_LAUNCH,        // Launch selected app
    BOOT_COMPLETE           // Boot finished, app running
};

// App metadata for menu display
struct AppInfo {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::string iconPath;        // Path to app icon sprite
    std::string splashPath;      // Path to app splash screen
    std::string executablePath;  // Path to app binary/library
    bool autoStart;         // Skip menu and auto-launch
    uint16_t screenWidth;   // App's preferred screen width
    uint16_t screenHeight;  // App's preferred screen height
    
    AppInfo() : autoStart(false), screenWidth(320), screenHeight(240) {}
};

// Device configuration for menu
struct DeviceConfig {
    // Display settings
    uint16_t brightness;        // 0-255
    uint8_t colorProfile;       // 0-3 color profiles
    bool vsyncEnabled;
    
    // Audio settings
    uint8_t masterVolume;       // 0-100
    bool audioEnabled;
    uint8_t audioProfile;       // 0=piezo, 1=i2s, 2=bluetooth
    
    // Network settings
    std::string wifiSSID;
    std::string wifiPassword;
    bool wifiEnabled;
    bool bluetoothEnabled;
    
    // System settings
    std::string deviceName;
    uint8_t sleepTimeout;       // Minutes before sleep
    bool debugMode;
    
    DeviceConfig() : brightness(255), colorProfile(0), vsyncEnabled(true),
                     masterVolume(80), audioEnabled(true), audioProfile(0),
                     wifiEnabled(false), bluetoothEnabled(false),
                     sleepTimeout(10), debugMode(false) {}
};

// Main bootloader class
class WispBootloader {
private:
    // Core systems
    LGFX* display;
    LazyResourceManager* resourceManager;
    AppLoopManager* appLoop;
    GraphicsEngine* graphics;
    WispCuratedAPI* api;
    WispSpriteLayerSystem* layerSystem;
    NativeAppLoader* appLoader;
    
    // Boot state
    BootStage currentStage;
    uint32_t stageStartTime;
    uint32_t splashDuration;
    bool fadeComplete;
    
    // App management
    static const int MAX_APPS = 50;
    AppInfo availableApps[MAX_APPS];
    int appCount;
    int selectedAppIndex;
    AppInfo currentApp;
    
    // Device configuration
    DeviceConfig deviceConfig;
    
    // Screen resolution (from board config)
    uint16_t deviceScreenWidth;
    uint16_t deviceScreenHeight;
    
    // Menu state
    bool menuActive;
    int menuSelection;
    enum MenuPage {
        MENU_MAIN,          // App launch + settings
        MENU_DISPLAY,       // Display settings
        MENU_AUDIO,         // Audio settings  
        MENU_NETWORK,       // WiFi/Bluetooth settings
        MENU_SYSTEM         // System settings
    } currentMenuPage;
    
    // Splash resources
    ResourceHandle wispLogo;
    ResourceHandle appSplash;
    
public:
    WispBootloader(LGFX* disp, LazyResourceManager* resMgr, AppLoopManager* loop, 
                   GraphicsEngine* gfx, WispCuratedAPI* apiPtr, NativeAppLoader* loader) :
        display(disp), resourceManager(resMgr), appLoop(loop), graphics(gfx), api(apiPtr),
        layerSystem(nullptr), appLoader(loader), currentStage(BOOT_HARDWARE_INIT), stageStartTime(0), splashDuration(2000),
        fadeComplete(false), selectedAppIndex(0), menuActive(false), menuSelection(0),
        currentMenuPage(MENU_MAIN), wispLogo(INVALID_RESOURCE), appSplash(INVALID_RESOURCE) {
        
        // Get screen resolution from board config
        deviceScreenWidth = SCREEN_WIDTH;
        deviceScreenHeight = SCREEN_HEIGHT;
    }
    
    // Initialize bootloader
    bool init();
    
    // Update boot sequence
    void update();
    
    // Render current boot stage
    void render();
    
    // Handle input during boot/menu
    void handleInput(const WispInputState& input);
    
    // Get current boot stage
    BootStage getCurrentStage() const { return currentStage; }
    bool isBootComplete() const { return currentStage == BOOT_COMPLETE; }
    
    // App management
    bool loadAppInfo(const std::string& appPath, AppInfo& info);
    void scanForApps();
    bool launchApp(int appIndex);
    
    // Menu system
    void showMainMenu();
    void showDisplayMenu();
    void showAudioMenu();
    void showNetworkMenu();
    void showSystemMenu();
    
    // Configuration management
    void loadDeviceConfig();
    void saveDeviceConfig();
    void applyDisplaySettings();
    void applyAudioSettings();
    
private:
    // Boot stage handlers
    void handleHardwareInit();
    void handleEngineInit();
    void handleSplashDisplay();
    void handleAppDiscovery();
    void handleAppSplash();
    void handleMenuCheck();
    void handleMenuDisplay();
    void handleAppLaunch();
    
    // Rendering helpers
    void renderWispSplash();
    void renderAppSplash();
    void renderMainMenu();
    void renderDisplayMenu();
    void renderAudioMenu();
    void renderNetworkMenu();
    void renderSystemMenu();
    void renderProgressBar(float progress);
    
    // Utility functions
    void advanceStage();
    bool isStageTimeout() const;
    void centerText(const std::string& text, int y, uint16_t color = 0xFFFF);
    void drawMenuItem(const std::string& text, int index, int y, bool selected);
    
    // App loading helpers
    bool parseAppManifest(const std::string& manifestPath, AppInfo& info);
    bool validateAppBinary(const std::string& binaryPath);
    
    // Animation helpers
    float getStageProgress() const;
    uint8_t calculateFadeAlpha() const;
};

// Implementation of key methods
inline bool WispBootloader::init() {
    ESP_LOGI(TAG, "Initializing Wisp Bootloader...");
    
    currentStage = BOOT_HARDWARE_INIT;
    stageStartTime = get_millis();
    splashDuration = 2000; // 2 seconds for splash
    
    // Load device configuration
    loadDeviceConfig();
    
    // Apply initial display settings
    display->setBrightness(deviceConfig.brightness);
    display->fillScreen(0x0000); // Black
    
    ESP_LOGI(TAG, "Wisp Bootloader initialized");
    return true;
}

inline void WispBootloader::update() {
    switch (currentStage) {
        case BOOT_HARDWARE_INIT:
            handleHardwareInit();
            break;
            
        case BOOT_ENGINE_INIT:
            handleEngineInit();
            break;
            
        case BOOT_SPLASH_DISPLAY:
            handleSplashDisplay();
            break;
            
        case BOOT_APP_DISCOVERY:
            handleAppDiscovery();
            break;
            
        case BOOT_APP_SPLASH:
            handleAppSplash();
            break;
            
        case BOOT_MENU_CHECK:
            handleMenuCheck();
            break;
            
        case BOOT_MENU_DISPLAY:
            handleMenuDisplay();
            break;
            
        case BOOT_APP_LAUNCH:
            handleAppLaunch();
            break;
            
        case BOOT_COMPLETE:
            // Boot complete - game loop takes over
            break;
    }
}

inline void WispBootloader::handleHardwareInit() {
    ESP_LOGI(TAG, "Initializing hardware...");
    
    // Hardware initialization happens here
    // Display, input, basic audio, etc.
    
    // Simulate initialization time
    if (get_millis() - stageStartTime > 500) {
        advanceStage();
    }
}

inline void WispBootloader::handleEngineInit() {
    ESP_LOGI(TAG, "Initializing engine systems...");
    
    // Initialize graphics engine
    graphics->init(display, nullptr); // Palette will be set later
    
    // Try to load enhanced LUT first (64x64 with dynamic slots)
    if (graphics->loadEnhancedLUT(lut_palette_lut)) {
        ESP_LOGI(TAG, "WISP Bootloader: Enhanced LUT system loaded");
        graphics->setUseEnhancedLUT(true);
    } else {
        // Fallback to legacy test LUT
        graphics->generateTestLUT(); // Basic color LUT
        graphics->setUseEnhancedLUT(false);
        ESP_LOGI(TAG, "WISP Bootloader: Using legacy LUT system");
    }
    
    // Initialize resource manager
    resourceManager->setMemoryBudget(128 * 1024); // 128KB
    
    // Initialize app loop
    appLoop->setTargetFPS(60.0f);
    appLoop->setAdaptiveLoading(true);
    
    // Initialize sprite layer system
    layerSystem = new WispSpriteLayerSystem(graphics);
    layerSystem->setViewport(deviceScreenWidth, deviceScreenHeight);
    g_LayerSystem = layerSystem; // Set global instance
    
    ESP_LOGI(TAG, "Sprite layer system initialized with 8 layers");
    
    advanceStage();
}

inline void WispBootloader::handleSplashDisplay() {
    // Show Wisp Engine logo for splashDuration
    if (get_millis() - stageStartTime > splashDuration) {
        advanceStage();
    }
}

inline void WispBootloader::handleAppDiscovery() {
    ESP_LOGI(TAG, "Scanning for applications...");
    
    scanForApps();
    
    ESP_LOGI(TAG, "Found %d applications", availableApps.size());
    
    advanceStage();
}

inline void WispBootloader::handleMenuCheck() {
    // Check if we should auto-start an app or show menu
    if (selectedAppIndex >= 0 && selectedAppIndex < (int)availableApps.size()) {
        if (availableApps[selectedAppIndex].autoStart) {
            ESP_LOGI(TAG, "Auto-starting application...");
            currentStage = BOOT_APP_LAUNCH;
            stageStartTime = get_millis();
            return;
        }
    }
    
    // Show menu
    menuActive = true;
    advanceStage();
}

inline void WispBootloader::handleMenuDisplay() {
    // Menu is active - input handling drives the menu
    // Stage doesn't advance until app is selected
}

inline void WispBootloader::render() {
    graphics->clearBuffers(0x0000); // Black background
    
    switch (currentStage) {
        case BOOT_HARDWARE_INIT:
        case BOOT_ENGINE_INIT:
            renderProgressBar(getStageProgress());
            centerText("Initializing Wisp Engine...", deviceScreenHeight / 2 + 20);
            break;
            
        case BOOT_SPLASH_DISPLAY:
            renderWispSplash();
            break;
            
        case BOOT_APP_DISCOVERY:
            renderProgressBar(getStageProgress());
            centerText("Scanning for applications...", deviceScreenHeight / 2 + 20);
            break;
            
        case BOOT_APP_SPLASH:
            renderAppSplash();
            break;
            
        case BOOT_MENU_CHECK:
            centerText("Loading menu...", deviceScreenHeight / 2);
            break;
            
        case BOOT_MENU_DISPLAY:
            if (menuActive) {
                switch (currentMenuPage) {
                    case MENU_MAIN: renderMainMenu(); break;
                    case MENU_DISPLAY: renderDisplayMenu(); break;
                    case MENU_AUDIO: renderAudioMenu(); break;
                    case MENU_NETWORK: renderNetworkMenu(); break;
                    case MENU_SYSTEM: renderSystemMenu(); break;
                }
            }
            break;
            
        case BOOT_APP_LAUNCH:
            centerText("Launching " + currentApp.name + "...", deviceScreenHeight / 2);
            renderProgressBar(getStageProgress());
            break;
            
        case BOOT_COMPLETE:
            // App is running - don't render anything
            break;
    }
    
    graphics->present();
}

inline void WispBootloader::renderWispSplash() {
    // Draw Wisp Engine logo centered
    centerText("WISP ENGINE", deviceScreenHeight / 2 - 40, 0xFFFF);
    centerText("v1.0", deviceScreenHeight / 2 - 20, 0x7BEF);
    
    // Draw fade animation based on stage progress
    float progress = getStageProgress();
    uint8_t alpha = (uint8_t)(255 * (1.0f - abs(progress * 2.0f - 1.0f)));
    
    // Simple logo placeholder
    graphics->drawRect(deviceScreenWidth / 2 - 32, deviceScreenHeight / 2 - 80, 
                      64, 64, 0x07E0, 5); // Green square logo
}

inline void WispBootloader::renderMainMenu() {
    centerText("WISP ENGINE", 20, 0xFFFF);
    centerText("Main Menu", 40, 0x7BEF);
    
    const int appAreaY = 70;
    const int appAreaHeight = 90;
    const int settingsY = appAreaY + appAreaHeight + 20;
    const int settingsHeight = 35;
    
    // App selection area
    if (appCount > 0) {
        renderAppSelectionArea(appAreaY, appAreaHeight, menuSelection == 0);
    } else {
        renderNoAppsMessage(appAreaY, appAreaHeight);
    }
    
    // Settings button
    renderSettingsButton(settingsY, settingsHeight, menuSelection == 1);
    
    // Navigation instructions
    renderMainMenuInstructions();
}

inline void WispBootloader::drawMenuItem(const std::string& text, int index, int y, bool selected) {
    uint16_t color = selected ? 0xFFE0 : 0xFFFF; // Yellow if selected, white otherwise
    uint16_t bgColor = selected ? 0x2104 : 0x0000; // Dark blue if selected, black otherwise
    
    if (selected) {
        graphics->drawRect(10, y - 2, deviceScreenWidth - 20, 20, bgColor, 3);
    }
    
    // Simple text rendering (would use actual text rendering in real implementation)
    // For now, just indicate position
    graphics->drawRect(20, y + 8, text.length() * 6, 2, color, 2);
}

inline void WispBootloader::advanceStage() {
    currentStage = (BootStage)((int)currentStage + 1);
    stageStartTime = get_millis();
    
    ESP_LOGI(TAG, "Boot stage advanced to: %d", (int)currentStage);
}

inline float WispBootloader::getStageProgress() const {
    uint32_t elapsed = get_millis() - stageStartTime;
    uint32_t stageDuration = (currentStage == BOOT_SPLASH_DISPLAY) ? splashDuration : 1000;
    return (float)elapsed / stageDuration;
}

inline void WispBootloader::centerText(const std::string& text, int y, uint16_t color) {
    // Simple centered text placeholder
    int textWidth = text.length() * 6; // Approximate character width
    int x = (deviceScreenWidth - textWidth) / 2;
    
    // Draw text as colored rectangles (placeholder for real text rendering)
    graphics->drawRect(x, y, textWidth, 8, color, 1);
}

inline void WispBootloader::scanForApps() {
    availableApps.clear();
    
    ESP_LOGI(TAG, "Scanning SPIFFS for .wisp files...");
    
    // Check if SPIFFS is mounted - ESP-IDF native
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS not found or failed to mount");
        return;
    }
    
    // Scan root directory for .wisp files using ESP-IDF
    DIR* root = opendir("/spiffs");
    if (!root) {
        ESP_LOGE(TAG, "Failed to open root directory");
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(root)) != NULL) {
        const char* fileName = entry->d_name;
        
        // Check if file has .wisp extension
        if (fileName.length() > 5 && fileName.substr(fileName.length() - 5) == ".wisp" && entry->d_type == DT_REG) {
            ESP_LOGI(TAG, "Found .wisp file: %s", fileName.c_str());
            ESP_LOGI("WISP", "%s", fileName.c_str());
            
            AppInfo appInfo;
            char fullPath[256];
            snprintf(fullPath, sizeof(fullPath), "/%s", fileName);
            
            // Try to load app metadata from the .wisp file
            if (loadAppInfo(fullPath, appInfo)) {
                appInfo.executablePath = fullPath;
                availableApps.push_back(appInfo);
                ESP_LOGI("WISP", "Added app: %s", appInfo.name.c_str());
            } else {
                // Create basic info from filename if metadata loading fails
                size_t dotPos = fileName.find_last_of('.');
                const char* dotPos = strrchr(fileName, '.');
                if (dotPos != nullptr) {
                    size_t len = dotPos - fileName;
                    strncpy(appInfo.name, fileName, len);
                    appInfo.name[len] = '\0';
                } else {
                    strcpy(appInfo.name, fileName);
                }
                appInfo.version = "Unknown";
                appInfo.author = "Unknown";
                appInfo.description = "Wisp application";
                appInfo.executablePath = fullPath;
                appInfo.autoStart = false;
                availableApps.push_back(appInfo);
                WISP_DEBUG_INFO("BOOTLOADER", "Added app");
            }
        }
    }
    
    closedir(root);
    
    // Also scan apps/ subdirectory if it exists
    DIR* appsDir = opendir("/spiffs/apps");
    if (appsDir) {
        ESP_LOGI("WISP", "Scanning /apps directory...");
        
        struct dirent* appEntry;
        while ((appEntry = readdir(appsDir)) != NULL) {
            String fileName = String(appEntry->d_name);
            
            if (fileName.length() > 5 && fileName.substring(fileName.length() - 5) == ".wisp" && appEntry->d_type == DT_REG) {
                ESP_LOGI("WISP", "Found .wisp file in /apps: %s", fileName.c_str());
                
                AppInfo appInfo;
                char fullPath[256];
                snprintf(fullPath, sizeof(fullPath), "/apps/%s", fileName);
                
                if (loadAppInfo(fullPath, appInfo)) {
                    appInfo.executablePath = fullPath;
                    availableApps.push_back(appInfo);
                    ESP_LOGI("WISP", "Added app: %s", appInfo.name.c_str());
                } else {
                    appInfo.name = fileName.substring(0, fileName.lastIndexOf('.'));
                    appInfo.version = "Unknown";
                    appInfo.author = "Unknown";
                    appInfo.description = "Wisp application";
                    appInfo.executablePath = fullPath;
                    appInfo.autoStart = false;
                    availableApps.push_back(appInfo);
                    ESP_LOGI("BOOTLOADER", "Added app (basic info): %s", appInfo.name.c_str());
                }
            }
        }
        
        closedir(appsDir);
    }
    
    ESP_LOGI("BOOTLOADER", "Scan complete. Found %zu .wisp applications", availableApps.size());
    
    // Set first app as default selection
    if (!availableApps.empty()) {
        selectedAppIndex = 0;
    }
}

inline bool WispBootloader::loadAppInfo(const char* appPath, AppInfo& info) {
    // For now, just extract basic info from filename
    // In the future, this could read metadata from the .wisp file header
    
    FILE* wispFile = fopen(appPath.c_str(), "rb");
    if (!wispFile) {
        ESP_LOGE("WISP", "Failed to open .wisp file: %s", appPath.c_str());
        return false;
    }
    
    // Get filename without path and extension - ESP-IDF native
    char fileName[256];
    strcpy(fileName, appPath);
    
    const char* lastSlash = strrchr(fileName, '/');
    if (lastSlash != nullptr) {
        strcpy(fileName, lastSlash + 1);
    }
    
    const char* lastDot = strrchr(fileName, '.');
    if (lastDot != nullptr) {
        fileName = fileName.substr(0, lastDot);
    }
    
    // Set basic info
    info.name = fileName;
    info.version = "1.0";
    info.author = "Unknown";
    info.description = "Wisp Application";
    info.iconPath = "";
    info.splashPath = "";
    info.autoStart = false;
    info.screenWidth = deviceScreenWidth;
    info.screenHeight = deviceScreenHeight;
    
    // TODO: Read actual metadata from .wisp file header
    // For now, we'll just use filename-based info
    
    fclose(wispFile);
    return true;
}

inline bool WispBootloader::launchApp(int appIndex) {
    if (appIndex < 0 || appIndex >= (int)availableApps.size()) {
        ESP_LOGE("BOOTLOADER", "Invalid app index");
        return false;
    }
    
    currentApp = availableApps[appIndex];
    
    ESP_LOGI("BOOTLOADER", "Launching app: %s", currentApp.name.c_str());
    ESP_LOGI("BOOTLOADER", "Path: %s", currentApp.executablePath.c_str());
    
    // Use the native app loader to load the .wisp file
    if (appLoader->loadApp(currentApp.executablePath)) {
        ESP_LOGI("BOOTLOADER", "App loaded successfully");
        currentStage = BOOT_COMPLETE;
        return true;
    } else {
        ESP_LOGE("BOOTLOADER", "Failed to load app");
        // Go back to menu
        currentStage = BOOT_MENU_DISPLAY;
        menuActive = true;
        return false;
    }
}

inline void WispBootloader::handleAppLaunch() {
    // Launch the selected app
    if (selectedAppIndex >= 0 && selectedAppIndex < (int)availableApps.size()) {
        if (launchApp(selectedAppIndex)) {
            ESP_LOGI("BOOTLOADER", "App launch completed");
        } else {
            ESP_LOGE("BOOTLOADER", "App launch failed, returning to menu");
        }
    } else {
        ESP_LOGW("BOOTLOADER", "No valid app selected, returning to menu");
        currentStage = BOOT_MENU_DISPLAY;
        menuActive = true;
    }
}

inline void WispBootloader::handleInput(const WispInputState& input) {
    if (currentStage != BOOT_MENU_DISPLAY || !menuActive) {
        return;
    }
    
    // Handle menu navigation based on current menu page
    switch (currentMenuPage) {
        case MENU_MAIN:
            // Main menu - navigate apps and settings
            if (input.up) {
                if (menuSelection > 0) {
                    menuSelection--;
                }
            }
            else if (input.down) {
                int maxSelection = 4; // Launch + 4 settings menus
                if (menuSelection < maxSelection) {
                    menuSelection++;
                }
            }
            else if (input.left) {
                // Cycle through available apps
                if (!availableApps.empty() && menuSelection == 0) {
                    selectedAppIndex--;
                    if (selectedAppIndex < 0) {
                        selectedAppIndex = (int)availableApps.size() - 1;
                    }
                }
            }
            else if (input.right) {
                // Cycle through available apps
                if (!availableApps.empty() && menuSelection == 0) {
                    selectedAppIndex++;
                    if (selectedAppIndex >= (int)availableApps.size()) {
                        selectedAppIndex = 0;
                    }
                }
            }
            else if (input.buttonA) {
                // Select menu item
                switch (menuSelection) {
                    case 0: // Launch app
                        if (!availableApps.empty()) {
                            currentStage = BOOT_APP_LAUNCH;
                            stageStartTime = get_millis();
                            menuActive = false;
                        }
                        break;
                    case 1: // Display settings
                        currentMenuPage = MENU_DISPLAY;
                        menuSelection = 0;
                        break;
                    case 2: // Audio settings
                        currentMenuPage = MENU_AUDIO;
                        menuSelection = 0;
                        break;
                    case 3: // Network settings
                        currentMenuPage = MENU_NETWORK;
                        menuSelection = 0;
                        break;
                    case 4: // System settings
                        currentMenuPage = MENU_SYSTEM;
                        menuSelection = 0;
                        break;
                }
            }
            break;
            
        case MENU_DISPLAY:
        case MENU_AUDIO:
        case MENU_NETWORK:
        case MENU_SYSTEM:
            // Sub-menus - just go back to main menu for now
            if (input.buttonB) {
                currentMenuPage = MENU_MAIN;
                menuSelection = 0;
            }
            break;
    }
}
