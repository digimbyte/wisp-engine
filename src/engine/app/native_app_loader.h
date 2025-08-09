#pragma once
// Native App Loader for .wash (Wisp Assembly Hybrid) binaries
// Handles loading, execution, and sandboxing of compiled ESP32 app code

#include "engine_common.h"
#include "interface.h"
#include "../scene/scene_system.h"
#include "../save/save_system.h"

namespace WispEngine {
namespace App {

// App binary format for .wash files
struct WashHeader {
    uint32_t magic;          // 'WASH' (0x48534157)
    uint32_t version;        // Binary format version
    uint32_t codeSize;       // Size of executable code
    uint32_t dataSize;       // Size of static data
    uint32_t entryPoint;     // Offset to app entry function
    uint32_t apiVersion;     // Required API version
    uint32_t memoryRequirement; // Memory needed for execution
    uint32_t reserved[9];    // Future expansion
};

// App sandbox for memory isolation
class AppSandbox {
private:
    uint8_t* sandboxMemory;    // Isolated memory region
    uint32_t memorySize;       // Total sandbox size
    uint32_t memoryUsed;       // Currently used memory
    uint32_t codeRegionSize;   // Size of code region
    uint32_t dataRegionSize;   // Size of data region
    
    // Memory regions within sandbox
    uint8_t* codeRegion;       // Executable code region
    uint8_t* dataRegion;       // Static data region
    uint8_t* heapRegion;       // Dynamic allocation region
    uint32_t heapSize;         // Remaining heap space
    
public:
    AppSandbox();
    ~AppSandbox();
    
    // Sandbox creation and destruction
    bool createSandbox(uint32_t totalSize);
    void destroySandbox();
    
    // Memory management within sandbox
    bool loadCode(const uint8_t* code, uint32_t size);
    bool loadData(const uint8_t* data, uint32_t size);
    void* sandboxMalloc(size_t size);
    void sandboxFree(void* ptr);
    
    // Memory validation
    bool isValidPointer(void* ptr);
    bool isValidCodePointer(void* ptr);
    bool isValidDataPointer(void* ptr);
    
    // Execution
    void* getEntryPoint(uint32_t offset);
    
    // Statistics
    uint32_t getUsedMemory() const { return memoryUsed; }
    uint32_t getFreeMemory() const { return memorySize - memoryUsed; }
    uint32_t getTotalSize() const { return memorySize; }
};

// Native app instance
class NativeApp {
private:
    WashHeader header;
    AppSandbox sandbox;
    WispAppBase* appInstance;
    
    // App metadata
    String appName;
    String appVersion;
    String appAuthor;
    
    // Execution state
    bool loaded;
    bool initialized;
    bool running;
    
    // API access
    WispCuratedAPI* api;
    SceneManager* sceneManager;
    SaveSystem* saveSystem;
    
    // Performance tracking
    uint32_t frameCount;
    uint32_t executionTime;
    uint32_t lastFrameTime;
    
public:
    NativeApp();
    ~NativeApp();
    
    // Loading and initialization
    bool loadFromBinary(const uint8_t* washData, size_t size);
    bool initialize(WispCuratedAPI* apiPtr, SceneManager* sceneMgr, SaveSystem* saveSys);
    void cleanup();
    
    // Execution control
    bool start();
    void pause();
    void resume();
    void stop();
    
    // Frame execution
    void update();
    void render();
    
    // Event handling
    void handleInput(const WispInputState& input);
    void handleSystemEvent(const String& event);
    
    // App information
    const String& getName() const { return appName; }
    const String& getVersion() const { return appVersion; }
    const String& getAuthor() const { return appAuthor; }
    
    // Status
    bool isLoaded() const { return loaded; }
    bool isInitialized() const { return initialized; }
    bool isRunning() const { return running; }
    
    // Performance
    uint32_t getFrameCount() const { return frameCount; }
    uint32_t getExecutionTime() const { return executionTime; }
    uint32_t getMemoryUsage() const { return sandbox.getUsedMemory(); }
};

// Function pointer types for app entry points
typedef WispAppBase* (*WashCreateFunction)(void);
typedef void (*WashDestroyFunction)(WispAppBase*);
typedef const char* (*WashGetInfoFunction)(void);

// Native app loader and manager
class NativeAppLoader {
private:
    static const uint8_t MAX_APPS = 4;  // Maximum concurrent native apps
    
    NativeApp apps[MAX_APPS];
    uint8_t activeApps;
    NativeApp* currentApp;
    
    // Memory management
    uint32_t totalMemoryBudget;
    uint32_t usedMemoryBudget;
    
    // Security and validation
    bool validateWashBinary(const uint8_t* data, size_t size);
    bool checkAPICompatibility(uint32_t requiredVersion);
    bool allocateMemoryBudget(uint32_t required);
    void releaseMemoryBudget(uint32_t amount);
    
public:
    NativeAppLoader();
    ~NativeAppLoader();
    
    // Configuration
    void setMemoryBudget(uint32_t totalBytes);
    
    // App loading
    NativeApp* loadApp(const uint8_t* washData, size_t size);
    bool initializeApp(NativeApp* app, WispCuratedAPI* api, 
                      SceneManager* sceneMgr, SaveSystem* saveSys);
    void unloadApp(NativeApp* app);
    
    // App management
    bool switchToApp(NativeApp* app);
    NativeApp* getCurrentApp() const { return currentApp; }
    uint8_t getActiveAppCount() const { return activeApps; }
    
    // System integration
    void updateCurrentApp();
    void renderCurrentApp();
    void handleInputForCurrentApp(const WispInputState& input);
    void handleSystemEvent(const String& event);
    
    // Memory management
    uint32_t getTotalMemoryUsage() const;
    uint32_t getAvailableMemory() const;
    void printMemoryStats() const;
    
    // Security
    bool validateAppPermissions(NativeApp* app);
    void enforceResourceLimits(NativeApp* app);
};

// Integration with WISP ROM loader
class WispNativeAppIntegration {
public:
    // Load native app from WISP ROM
    static NativeApp* loadFromROM(WispSegmentedLoader* romLoader, 
                                 const String& binaryName = "main.wash");
    
    // Initialize app with ROM assets and scenes
    static bool initializeWithROM(NativeApp* app, WispSegmentedLoader* romLoader,
                                 WispCuratedAPI* api, SceneManager* sceneMgr, 
                                 SaveSystem* saveSys);
    
    // Load scenes from ROM into scene manager
    static bool loadScenesFromROM(WispSegmentedLoader* romLoader, 
                                 SceneManager* sceneMgr);
    
    // Set up asset streaming for app
    static bool setupAssetStreaming(NativeApp* app, WispSegmentedLoader* romLoader);
};

// Inline implementations for performance

inline bool AppSandbox::isValidPointer(void* ptr) {
    uint8_t* p = static_cast<uint8_t*>(ptr);
    return (p >= sandboxMemory && p < sandboxMemory + memorySize);
}

inline bool AppSandbox::isValidCodePointer(void* ptr) {
    uint8_t* p = static_cast<uint8_t*>(ptr);
    return (p >= codeRegion && p < codeRegion + codeRegionSize);
}

inline bool AppSandbox::isValidDataPointer(void* ptr) {
    uint8_t* p = static_cast<uint8_t*>(ptr);
    return (p >= dataRegion && p < dataRegion + dataRegionSize);
}

inline void* AppSandbox::getEntryPoint(uint32_t offset) {
    if (offset >= codeRegionSize) return nullptr;
    return codeRegion + offset;
}

inline uint32_t NativeAppLoader::getTotalMemoryUsage() const {
    return usedMemoryBudget;
}

inline uint32_t NativeAppLoader::getAvailableMemory() const {
    return totalMemoryBudget - usedMemoryBudget;
}

} // namespace App
} // namespace WispEngine
