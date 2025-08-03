// app_manager.h
#pragma once
#include "esp32_common.h"
#include <vector>
#include "../engine/app/loader.h"
#include "app_loop_manager.h"

// App information structure
struct AppInfo {
    String name;
    String version;
    String author;
    String description;
    String iconPath;
    String splashPath;
    String executablePath;
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
    
    String currentAppName;
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
    bool loadApp(const String& appName) {
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
            appLoopManager->getAppLoop()->entities.clear();
            appLoopManager->getAppLoop()->regions.clear();
            appLoopManager->getAppLoop()->frameEvents.clear();
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
    String getCurrentAppName() const { return currentAppName; }
    
    // SD card app discovery
    void scanForApps() {
        availableApps.clear();
        
        WISP_DEBUG_INFO("APP_MANAGER", "Scanning SD card for .wisp files...");
        
        // Check if SD card is available
        if (!SD.begin()) {
            WISP_DEBUG_ERROR("APP_MANAGER", "SD card not found or failed to mount");
            return;
        }
        
        // Scan root directory for .wisp files
        File root = SD.open("/");
        if (!root) {
            WISP_DEBUG_ERROR("APP_MANAGER", "Failed to open root directory");
            return;
        }
        
        File file = root.openNextFile();
        while (file) {
            String fileName = file.name();
            
            // Check if file has .wisp extension
            if (fileName.endsWith(".wisp") && !file.isDirectory()) {
                WISP_DEBUG_INFO("APP_MANAGER", "Found .wisp file");
                
                AppInfo appInfo;
                String fullPath = "/" + fileName;
                
                // Create basic info from filename
                appInfo.name = fileName.substring(0, fileName.lastIndexOf('.'));
                appInfo.version = "1.0";
                appInfo.author = "Unknown";
                appInfo.description = "Wisp Application";
                appInfo.executablePath = fullPath;
                appInfo.autoStart = false;
                availableApps.push_back(appInfo);
                
                WISP_DEBUG_INFO("APP_MANAGER", "Added app");
            }
            
            file.close();
            file = root.openNextFile();
        }
        
        root.close();
        
        // Also scan apps/ subdirectory if it exists
        File appsDir = SD.open("/apps");
        if (appsDir) {
            WISP_DEBUG_INFO("APP_MANAGER", "Scanning /apps directory...");
            
            File appFile = appsDir.openNextFile();
            while (appFile) {
                String fileName = appFile.name();
                
                if (fileName.endsWith(".wisp") && !appFile.isDirectory()) {
                    WISP_DEBUG_INFO("APP_MANAGER", "Found .wisp file in /apps");
                    
                    AppInfo appInfo;
                    String fullPath = "/apps/" + fileName;
                    
                    appInfo.name = fileName.substring(0, fileName.lastIndexOf('.'));
                    appInfo.version = "1.0";
                    appInfo.author = "Unknown";
                    appInfo.description = "Wisp Application";
                    appInfo.executablePath = fullPath;
                    appInfo.autoStart = false;
                    availableApps.push_back(appInfo);
                    
                    WISP_DEBUG_INFO("APP_MANAGER", "Added app");
                }
                
                appFile.close();
                appFile = appsDir.openNextFile();
            }
            
            appsDir.close();
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
        if (index < 0 || index >= (int)availableApps.size()) {
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
