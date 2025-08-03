// engine/native_app_loader.h - ESP32-C6/S3 Native App Loader using ESP-IDF
// Native C++ app loader for ESP32 - loads compiled app binaries and manages lifecycle
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include "interface.h"

// Native C++ app loader - loads compiled app binaries and manages lifecycle
class NativeAppLoader {
private:
    WispApp* currentApp;
    AppCreateFunction createFunc;
    AppDestroyFunction destroyFunc;
    String currentAppPath;
    bool appLoaded;
    
    // App performance monitoring
    uint32_t appStartTime;
    uint32_t frameUpdateTime;
    uint32_t frameRenderTime;
    uint32_t totalFrames;
    
public:
    NativeAppLoader() : 
        currentApp(nullptr),
        createFunc(nullptr),
        destroyFunc(nullptr),
        appLoaded(false),
        appStartTime(0),
        frameUpdateTime(0),
        frameRenderTime(0),
        totalFrames(0) {}
    
    ~NativeAppLoader() {
        unloadApp();
    }
    
    // Load compiled app from file
    bool loadApp(const String& appPath);
    
    // Load app from memory (for built-in apps)
    bool loadAppFromMemory(AppCreateFunction createFn, AppDestroyFunction destroyFn);
    
    // Unload current app
    void unloadApp();
    
    // App lifecycle management
    bool initializeApp(EngineCore* engine);
    void updateApp(float deltaTime);
    void renderApp(GraphicsEngine* graphics);
    void cleanupApp();
    
    // Input forwarding
    void forwardButtonPress(uint8_t button);
    void forwardButtonRelease(uint8_t button);
    void forwardAccelerometer(float x, float y, float z);
    
    // Event forwarding
    void forwardEntityCollision(uint16_t entity1, uint16_t entity2);
    void forwardTriggerEnter(uint16_t entityId, uint16_t triggerId);
    void forwardTriggerExit(uint16_t entityId, uint16_t triggerId);
    
    // App information
    String getAppName() const;
    String getAppVersion() const;
    String getAppAuthor() const;
    String getAppPath() const { return currentAppPath; }
    
    // Performance settings
    uint8_t getTargetFPS() const;
    uint8_t getMinimumFPS() const;
    uint32_t getMaxMemoryKB() const;
    bool allowAdaptiveFrameRate() const;
    
    // Status
    bool isAppLoaded() const { return appLoaded && currentApp != nullptr; }
    WispApp* getCurrentApp() const { return currentApp; }
    
    // Performance monitoring
    uint32_t getFrameUpdateTime() const { return frameUpdateTime; }
    uint32_t getFrameRenderTime() const { return frameRenderTime; }
    uint32_t getTotalFrames() const { return totalFrames; }
    uint32_t getAppRuntime() const;
    
    void printAppPerformanceReport() const;
    void resetPerformanceCounters();
    
private:
    bool validateApp(WispApp* app);
    void logAppEvent(const String& event);
};

// Implementation of key methods
inline bool NativeAppLoader::loadAppFromMemory(AppCreateFunction createFn, AppDestroyFunction destroyFn) {
    if (!createFn || !destroyFn) {
        ESP_LOGE("APP_LOADER", "Invalid app functions");
        return false;
    }
    
    // Unload current app first
    unloadApp();
    
    createFunc = createFn;
    destroyFunc = destroyFn;
    
    // Create app instance
    currentApp = createFunc();
    if (!currentApp) {
        ESP_LOGE("APP_LOADER", "Failed to create app instance");
        return false;
    }
    
    if (!validateApp(currentApp)) {
        destroyFunc(currentApp);
        currentApp = nullptr;
        return false;
    }
    
    appLoaded = true;
    currentAppPath = "memory://builtin";
    
    logAppEvent("App loaded from memory");
    return true;
}

inline void NativeAppLoader::unloadApp() {
    if (currentApp) {
        logAppEvent("Unloading app");
        
        // Cleanup app
        currentApp->cleanup();
        
        // Destroy app instance
        if (destroyFunc) {
            destroyFunc(currentApp);
        } else {
            delete currentApp; // Fallback
        }
        
        currentApp = nullptr;
    }
    
    createFunc = nullptr;
    destroyFunc = nullptr;
    appLoaded = false;
    currentAppPath = "";
    
    resetPerformanceCounters();
}

inline bool NativeAppLoader::initializeApp(EngineCore* engine) {
    if (!currentApp || !engine) {
        return false;
    }
    
    logAppEvent("Initializing app");
    
    bool success = currentApp->init(engine);
    if (success) {
        appStartTime = millis();
        totalFrames = 0;
        
        ESP_LOGI("APP_LOADER", "App initialized: %s v%s by %s", 
                getAppName().c_str(), getAppVersion().c_str(), getAppAuthor().c_str());
    } else {
        ESP_LOGE("APP_LOADER", "App initialization failed");
    }
    
    return success;
}

inline void NativeAppLoader::updateApp(float deltaTime) {
    if (!currentApp) return;
    
    uint32_t start = micros();
    currentApp->update(deltaTime);
    frameUpdateTime = micros() - start;
    
    totalFrames++;
}

inline void NativeAppLoader::renderApp(GraphicsEngine* graphics) {
    if (!currentApp || !graphics) return;
    
    uint32_t start = micros();
    currentApp->render(graphics);
    frameRenderTime = micros() - start;
}

inline void NativeAppLoader::cleanupApp() {
    if (currentApp) {
        logAppEvent("Cleaning up app");
        currentApp->cleanup();
    }
}

inline void NativeAppLoader::forwardButtonPress(uint8_t button) {
    if (currentApp) {
        currentApp->onButtonPress(button);
    }
}

inline void NativeAppLoader::forwardButtonRelease(uint8_t button) {
    if (currentApp) {
        currentApp->onButtonRelease(button);
    }
}

inline void NativeAppLoader::forwardEntityCollision(uint16_t entity1, uint16_t entity2) {
    if (currentApp) {
        currentApp->onEntityCollision(entity1, entity2);
    }
}

inline String NativeAppLoader::getAppName() const {
    return currentApp ? String(currentApp->getAppName()) : "No App";
}

inline String NativeAppLoader::getAppVersion() const {
    return currentApp ? String(currentApp->getAppVersion()) : "0.0.0";
}

inline String NativeAppLoader::getAppAuthor() const {
    return currentApp ? String(currentApp->getAppAuthor()) : "Unknown";
}

inline uint8_t NativeAppLoader::getTargetFPS() const {
    return currentApp ? currentApp->getTargetFPS() : 30;
}

inline uint8_t NativeAppLoader::getMinimumFPS() const {
    return currentApp ? currentApp->getMinimumFPS() : 15;
}

inline bool NativeAppLoader::validateApp(WispApp* app) {
    if (!app) return false;
    
    // Basic validation
    if (!app->getAppName() || strlen(app->getAppName()) == 0) {
        ESP_LOGE("APP_LOADER", "App must provide a name");
        return false;
    }
    
    if (!app->getAppVersion() || strlen(app->getAppVersion()) == 0) {
        ESP_LOGE("APP_LOADER", "App must provide a version");
        return false;
    }
    
    if (!app->getAppAuthor() || strlen(app->getAppAuthor()) == 0) {
        ESP_LOGE("APP_LOADER", "App must provide an author");
        return false;
    }
    
    return true;
}

inline uint32_t NativeAppLoader::getAppRuntime() const {
    return appStartTime > 0 ? (millis() - appStartTime) : 0;
}

inline void NativeAppLoader::logAppEvent(const String& event) {
    if (!currentAppPath.empty()) {
        ESP_LOGI("APP_LOADER", "%s (%s)", event.c_str(), currentAppPath.c_str());
    } else {
        ESP_LOGI("APP_LOADER", "%s", event.c_str());
    }
}

inline void NativeAppLoader::printAppPerformanceReport() const {
    ESP_LOGI("APP_PERF", "=== App Performance Report ===");
    ESP_LOGI("APP_PERF", "App: %s v%s", getAppName().c_str(), getAppVersion().c_str());
    ESP_LOGI("APP_PERF", "Author: %s", getAppAuthor().c_str());
    ESP_LOGI("APP_PERF", "Runtime: %.2f seconds", getAppRuntime() / 1000.0f);
    ESP_LOGI("APP_PERF", "Total Frames: %lu", totalFrames);
    ESP_LOGI("APP_PERF", "Avg Update Time: %lu μs", frameUpdateTime);
    ESP_LOGI("APP_PERF", "Avg Render Time: %lu μs", frameRenderTime);
    ESP_LOGI("APP_PERF", "Target FPS: %u", getTargetFPS());
    ESP_LOGI("APP_PERF", "==============================");
}
