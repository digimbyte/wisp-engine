// app_lifecycle.h
// WISP Engine App Lifecycle Manager
// Handles app loading, layout initialization, save state loading, and loop control
#pragma once

#include "engine_common.h"
#include "interface.h"
#include "loop_manager.h"
#include "wisp_segmented_loader.h"
#include "../scene/scene_system.h"
#include "../database/save_system.h"

namespace WispEngine {

// App loading phases
enum class AppLoadPhase : uint8_t {
    UNLOADED,           // No app loaded
    LOADING_ROM,        // Loading WISP ROM file
    LOADING_LOGIC,      // Loading and initializing app logic
    LOADING_LAYOUTS,    // Loading scene layouts and panels
    INITIALIZING_BASE,  // Setting up base/initial state
    LOADING_SAVE_DATA,  // Loading save game data over base state
    READY,              // App fully loaded and ready to run
    RUNNING,            // App is actively running
    PAUSED,             // App is paused but loaded
    UNLOADING,          // App is being unloaded
    ERROR               // Error occurred during loading
};

// Save loading strategy
enum class SaveLoadStrategy : uint8_t {
    NEW_GAME,           // Start fresh, ignore saves
    CONTINUE_GAME,      // Load from save if available, otherwise new game
    LOAD_SPECIFIC       // Load a specific save slot
};

// App initialization data passed to apps
struct AppInitData {
    SceneManager* sceneManager;     // Scene management system
    WispSaveSystem* saveSystem;     // Save/load system
    WispCuratedAPI* api;            // Curated API access
    bool isNewGame;                 // True if starting fresh, false if loaded from save
    uint8_t saveSlot;               // Save slot being loaded (0 = auto-save)
    
    AppInitData() : sceneManager(nullptr), saveSystem(nullptr), api(nullptr), 
                   isNewGame(true), saveSlot(0) {}
};

// App lifecycle manager
class AppLifecycleManager {
private:
    // Core systems
    WispSegmentedLoader* romLoader;
    SceneManager* sceneManager;
    WispSaveSystem* saveSystem;
    GameLoopManager* loopManager;
    WispCuratedAPI* curatedAPI;
    
    // Current state
    AppLoadPhase currentPhase;
    WispAppBase* currentApp;
    bool loopFrozen;
    
    // ROM and app data
    String currentRomPath;
    AppInitData initData;
    
    // Loading configuration
    SaveLoadStrategy saveStrategy;
    uint8_t targetSaveSlot;
    bool autoSaveEnabled;
    uint32_t autoSaveInterval;
    
    // Performance tracking
    uint32_t loadStartTime;
    uint32_t phaseStartTime;
    uint32_t phaseDurations[static_cast<uint8_t>(AppLoadPhase::ERROR) + 1];
    
public:
    AppLifecycleManager();
    ~AppLifecycleManager();
    
    // === INITIALIZATION ===
    bool initialize(WispSegmentedLoader* loader, SceneManager* sceneMgr, 
                   WispSaveSystem* saveSys, GameLoopManager* loopMgr, 
                   WispCuratedAPI* api);
    void shutdown();
    
    // === APP LOADING ===
    bool loadApp(const String& romPath, SaveLoadStrategy strategy = SaveLoadStrategy::CONTINUE_GAME, 
                uint8_t saveSlot = 0);
    bool loadAppFromMemory(WispAppCreateFunction createFn, WispAppDestroyFunction destroyFn,
                          SaveLoadStrategy strategy = SaveLoadStrategy::CONTINUE_GAME, 
                          uint8_t saveSlot = 0);
    void unloadApp();
    
    // === LIFECYCLE CONTROL ===
    bool startApp();        // Unfreeze loop and begin running
    void pauseApp();        // Pause app but keep loaded
    void resumeApp();       // Resume from pause
    void freezeLoop();      // Freeze game loop (for loading)
    void unfreezeLoop();    // Unfreeze game loop (start running)
    
    // === SAVE SYSTEM INTEGRATION ===
    bool saveGame(uint8_t slot = 0);
    bool loadSave(uint8_t slot = 0);
    bool hasSave(uint8_t slot = 0);
    bool deleteSave(uint8_t slot = 0);
    void setAutoSave(bool enabled, uint32_t intervalMs = 30000);
    
    // === STATE QUERIES ===
    AppLoadPhase getCurrentPhase() const { return currentPhase; }
    bool isAppLoaded() const { return currentPhase >= AppLoadPhase::READY; }
    bool isAppRunning() const { return currentPhase == AppLoadPhase::RUNNING && !loopFrozen; }
    bool isLoopFrozen() const { return loopFrozen; }
    WispAppBase* getCurrentApp() const { return currentApp; }
    
    // === PERFORMANCE MONITORING ===
    uint32_t getLoadTime() const;
    uint32_t getPhaseTime(AppLoadPhase phase) const;
    void printLoadingReport() const;
    
    // === UPDATE (call from main loop) ===
    void update();
    
private:
    // === INTERNAL LOADING PHASES ===
    bool executeLoadingPhase();
    bool phaseLoadRom();
    bool phaseLoadLogic();
    bool phaseLoadLayouts();
    bool phaseInitializeBase();
    bool phaseLoadSaveData();
    bool phaseReady();
    
    // === ROM LOADING HELPERS ===
    bool loadLayoutsFromRom();
    bool loadPanelsFromRom(uint8_t layoutIndex, const String& layoutName);
    bool loadEntitiesFromRom(uint8_t layoutIndex, uint8_t panelIndex, const String& panelName);
    bool loadTilesFromRom(uint8_t layoutIndex, uint8_t panelIndex, const String& panelName);
    bool loadBackgroundFromRom(uint8_t layoutIndex, uint8_t panelIndex, const String& panelName);
    
    // === STATE MANAGEMENT ===
    bool setupAppSaveSystem();
    bool initializeAppBaseState();
    bool loadSaveDataOverState();
    bool validateAppState();
    
    // === ERROR HANDLING ===
    void setError(const String& message);
    void transitionToPhase(AppLoadPhase newPhase);
    void resetLoadingState();
    
    // === UTILITY ===
    String getPhaseDescription(AppLoadPhase phase) const;
    void logPhaseTransition(AppLoadPhase from, AppLoadPhase to);
    uint32_t getCurrentTime() const;
};

// Global app lifecycle manager instance
extern AppLifecycleManager appLifecycle;

    // === CURATED API INTEGRATION ===
    bool integrateWithCuratedAPI();
    void exposeSaveSystemToAPI();
    void exposeSceneSystemToAPI();

// === CONVENIENCE MACROS ===
#define APP_IS_LOADING() (appLifecycle.getCurrentPhase() < AppLoadPhase::READY)
#define APP_IS_READY() (appLifecycle.getCurrentPhase() >= AppLoadPhase::READY)
#define APP_IS_RUNNING() (appLifecycle.isAppRunning())
#define APP_FREEZE_LOOP() (appLifecycle.freezeLoop())
#define APP_UNFREEZE_LOOP() (appLifecycle.unfreezeLoop())

} // namespace WispEngine
