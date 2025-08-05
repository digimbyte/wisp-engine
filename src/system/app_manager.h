// app_manager.h
#pragma once
#include "esp32_common.h"
#include <vector>
#include <string>
#include <dirent.h>

// Include debug macros FIRST to avoid ordering issues
#include "../engine/core/debug.h"

#include "../engine/app/loader.h"
#include "../engine/app/loop_manager.h"

// App information structure
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
    
    // Updated for ESP32-C6-LCD-1.47 actual screen size
    AppInfo() : autoStart(false), screenWidth(172), screenHeight(320) {}
};
// App Manager - Handles loading and execution of C++ applications
class AppManager {
private:
    AppLoader* appLoader;
    AppLoopManager* appLoopManager;
    
    std::string currentAppName;
    bool appRunning;
    bool appInitialized;
    
    // Available apps discovered from SD card
    static const int MAX_APPS = 50;
    AppInfo availableApps[MAX_APPS];
    int appCount;
    
public:
    AppManager() : appLoader(nullptr), appLoopManager(nullptr),
                   currentAppName(""), appRunning(false), appInitialized(false) {}
    
    // Initialize the app manager
    bool init(AppLoader* loader, AppLoopManager* appLoop) {
        if (!loader || !appLoop) {
            WISP_DEBUG_ERROR("APP_MANAGER", "Invalid references for App Manager");
            return false;
        }
        
        appLoader = loader;
        appLoopManager = appLoop;
        
        WISP_DEBUG_INFO("APP_MANAGER", "App Manager initialized for C++ applications");
        return true;
    }
    
    // Load and start a C++ application
    bool loadApp(const std::string& appName) {
        if (appRunning) {
            WISP_DEBUG_ERROR("APP_MANAGER", "Another app is already running");
            return false;
        }
        
        if (!appLoader->loadApp(appName)) {
            WISP_DEBUG_ERROR("APP_MANAGER", "Failed to load app");
            return false;
        }
        
        currentAppName = appName;
        appInitialized = false;
        appRunning = true;
        
        WISP_DEBUG_INFO("APP_MANAGER", "C++ App loaded");
        return true;
    }
    
    // Stop the current application
    void stopApp() {
        if (!appRunning) return;
        
        // Reset app loop state
        if (appLoopManager->getAppLoop()) {
            // TODO: Reset app state when proper entity system is implemented
            // appLoopManager->getAppLoop()->entities.clear();
            // appLoopManager->getAppLoop()->regions.clear();
            // appLoopManager->getAppLoop()->frameEvents.clear();
        }
        
        currentAppName = "";
        appRunning = false;
        appInitialized = false;
        
        WISP_DEBUG_INFO("APP_MANAGER", "App stopped");
    }
    
    // Update the current application
    void update() {
        if (!appRunning) return;
        
        // Initialize app on first update
        if (!appInitialized) {
            initializeApp();
        }
        
        // C++ applications handle their own update logic
        // through the app loop manager
    }
    
    // Handle collision events (C++ applications handle these directly)
    void onCollision(uint16_t entityId, uint16_t otherEntityId, uint16_t regionId) {
        if (!appRunning || !appInitialized) return;
        // C++ applications register collision handlers directly with the app loop
    }
    
    // Handle trigger events (C++ applications handle these directly)
    void onTriggerEnter(uint16_t entityId, uint16_t regionId) {
        if (!appRunning || !appInitialized) return;
        // C++ applications register trigger handlers directly with the app loop
    }
    
    void onTriggerExit(uint16_t entityId, uint16_t regionId) {
        if (!appRunning || !appInitialized) return;
        // C++ applications register trigger handlers directly with the app loop
    }
    
    // Query functions
    bool isAppRunning() const { return appRunning; }
    std::string getCurrentAppName() const { return currentAppName; }
    
    // SD card app discovery
    void scanForApps() {
        appCount = 0;  // Reset the app count instead of calling clear()
        
        WISP_DEBUG_INFO("APP_MANAGER", "Scanning SD card for .wisp files...");
        
        // Check if SD card is available
        if (!SD.begin()) {
            WISP_DEBUG_ERROR("APP_MANAGER", "SD card not found or failed to mount");
            return;
        }
        
        // Scan root directory for .wisp files
        DIR* root = opendir("/");
        if (!root) {
            WISP_DEBUG_ERROR("APP_MANAGER", "Failed to open root directory");
            return;
        }
        
        struct dirent* entry;
        while ((entry = readdir(root)) != nullptr) {
            if (entry->d_type == DT_REG) { // Regular file
                String fileName(entry->d_name);
                
                // Check if file has .wisp extension
                if (fileName.indexOf(".wisp") != -1) {
                    WISP_DEBUG_INFO("APP_MANAGER", "Found .wisp file");
                    
                    AppInfo appInfo;
                    std::string fullPath = std::string("/") + fileName.c_str();
                    
                    // Create basic info from filename
                    appInfo.name = fileName.substring(0, fileName.lastIndexOf('.')).c_str();
                    appInfo.version = "1.0";
                    appInfo.author = "Unknown";
                    appInfo.description = "Wisp Application";
                    appInfo.executablePath = fullPath;
                    appInfo.autoStart = false;
                    if (appCount < MAX_APPS) {
                        availableApps[appCount] = appInfo;
                        appCount++;
                    }
                    
                    WISP_DEBUG_INFO("APP_MANAGER", "Added app");
                }
            }
        }
        
        closedir(root);
        
        // Also scan apps/ subdirectory if it exists
        DIR* appsDir = opendir("/apps");
        if (appsDir) {
            WISP_DEBUG_INFO("APP_MANAGER", "Scanning /apps directory...");
            
            while ((entry = readdir(appsDir)) != nullptr) {
                if (entry->d_type == DT_REG) { // Regular file
                    String fileName(entry->d_name);
                    
                    if (fileName.indexOf(".wisp") != -1) {
                        WISP_DEBUG_INFO("APP_MANAGER", "Found .wisp file in /apps");
                        
                        AppInfo appInfo;
                        std::string fullPath = std::string("/apps/") + fileName.c_str();
                        
                        appInfo.name = fileName.substring(0, fileName.lastIndexOf('.')).c_str();
                        appInfo.version = "1.0";
                        appInfo.author = "Unknown";
                        appInfo.description = "Wisp Application";
                        appInfo.executablePath = fullPath;
                        appInfo.autoStart = false;
                        if (appCount < MAX_APPS) {
                            availableApps[appCount] = appInfo;
                            appCount++;
                        }
                        
                        WISP_DEBUG_INFO("APP_MANAGER", "Added app");
                    }
                }
            }
            
            closedir(appsDir);
        }
        
        WISP_DEBUG_INFO("APP_MANAGER", "Scan complete");
    }
    
    // Get available apps
    const AppInfo* getAvailableApps(int* count) const { 
        *count = appCount; 
        return availableApps; 
    }
    
    // Launch app by index
    bool launchAppByIndex(int index) {
        if (index < 0 || index >= appCount) {
            WISP_DEBUG_ERROR("APP_MANAGER", "Invalid app index");
            return false;
        }
        
        return loadApp(availableApps[index].executablePath);
    }
    
    // Cleanup
    ~AppManager() {
        // C++ app cleanup handled automatically
    }
    
private:
    void initializeApp() {
        // C++ applications are initialized through their constructors
        // and the app loop manager
        appInitialized = true;
        WISP_DEBUG_INFO("APP_MANAGER", "C++ App initialized");
    }
};
