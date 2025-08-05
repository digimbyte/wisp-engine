// engine/app/interface.h
// app_interface.h - ESP32-C6/S3 App Interface using ESP-IDF
// Curated API interface for ESP32 application development
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include "curated_api.h"

// Base class for all Wisp Engine applications
// Apps MUST inherit from this and are restricted to the curated API

class WispAppBase {
protected:
    WispCuratedAPI* api;        // The ONLY way apps can access engine features
    bool initialized;
    bool active;
    
    // App metadata
    String appName;
    String appVersion;
    String appAuthor;
    
    // Performance tracking
    uint32_t frameCount;
    uint32_t startTime;
    
public:
    WispAppBase() : api(nullptr), initialized(false), active(false),
                    appName("Unknown App"), appVersion("1.0"), appAuthor("Unknown"),
                    frameCount(0), startTime(0) {}
    
    virtual ~WispAppBase() {}
    
    // Called by engine to provide API access
    void setAPI(WispCuratedAPI* apiPtr) { 
        api = apiPtr; 
    }
    
    // App lifecycle - apps MUST implement these
    virtual bool init() = 0;        // Initialize app resources
    virtual void update() = 0;      // Update game logic (called every frame)
    virtual void render() = 0;      // Render graphics (called every frame)
    virtual void cleanup() = 0;     // Clean up when app is unloaded
    
    // Optional lifecycle hooks
    virtual void onPause() {}       // Called when app is paused
    virtual void onResume() {}      // Called when app is resumed
    virtual void onLowMemory() {}   // Called when system is low on memory
    virtual void onError(const String& error) {} // Called when error occurs
    
    // App metadata (apps can override)
    virtual String getName() const { return appName; }
    virtual String getVersion() const { return appVersion; }
    virtual String getAuthor() const { return appAuthor; }
    
    // State management
    bool isInitialized() const { return initialized; }
    bool isActive() const { return active; }
    uint32_t getFrameCount() const { return frameCount; }
    uint32_t getRunTime() const { return get_millis() - startTime; }
    
    // Called by engine - apps should not call these
    bool internalInit() {
        if (initialized) return true;
        
        startTime = get_millis();
        frameCount = 0;
        
        bool result = init();
        if (result) {
            initialized = true;
            active = true;
        }
        return result;
    }
    
    void internalUpdate() {
        if (!active || !initialized || !api) return;
        
        if (!api->beginFrame()) {
            // Emergency mode - don't update
            return;
        }
        
        api->beginUpdate();
        update();
        api->endUpdate();
        
        frameCount++;
    }
    
    void internalRender() {
        if (!active || !initialized || !api) return;
        
        api->beginRender();
        render();
        api->endRender();
        
        api->endFrame();
    }
    
    void internalPause() {
        if (active) {
            active = false;
            onPause();
        }
    }
    
    void internalResume() {
        if (!active && initialized) {
            active = true;
            onResume();
        }
    }
    
    void internalCleanup() {
        if (initialized) {
            cleanup();
            initialized = false;
            active = false;
        }
    }
    
    void internalLowMemory() {
        onLowMemory();
    }
    
    void internalError(const String& error) {
        api->printError("App Error: " + error);
        onError(error);
    }
    
protected:
    // Helper functions for apps (these use the curated API)
    void setAppInfo(const String& name, const String& version, const String& author) {
        appName = name;
        appVersion = version;
        appAuthor = author;
    }
    
    // Quick access to common API functions
    const WispInputState& input() const { return api->getInput(); }
    uint32_t time() const { return api->getTime(); }
    uint32_t deltaTime() const { return api->getDeltaTime(); }
    
    bool draw(ResourceHandle sprite, float x, float y) {
        return api->drawSprite(sprite, x, y);
    }
    
    bool drawAt(ResourceHandle sprite, float x, float y, uint8_t depth) {
        return api->drawSprite(sprite, x, y, depth);
    }
    
    EntityHandle entity() { return api->createEntity(); }
    
    void print(const String& msg) { api->print(msg); }
    void warning(const String& msg) { api->printWarning(msg); }
    void error(const String& msg) { api->printError(msg); }
    
    // Assert macro for debug builds
    void assertApi(bool condition, const String& message) {
        if (!condition) {
            error("ASSERTION FAILED: " + message);
        }
    }
};

// App factory function type for dynamic loading
typedef WispAppBase* (*WispAppCreateFunction)();
typedef void (*WispAppDestroyFunction)(WispAppBase*);

// Macro to register an app
#define WISP_REGISTER_APP(AppClass) \
    extern "C" { \
        WispAppBase* createWispApp() { \
            return new AppClass(); \
        } \
        void destroyWispApp(WispAppBase* app) { \
            delete app; \
        } \
        const char* getWispAppName() { \
            static AppClass tempApp; \
            return tempApp.getName().c_str(); \
        } \
        const char* getWispAppVersion() { \
            static AppClass tempApp; \
            return tempApp.getVersion().c_str(); \
        } \
        const char* getWispAppAuthor() { \
            static AppClass tempApp; \
            return tempApp.getAuthor().c_str(); \
        } \
    }
