// engine/game_loop_manager.h - ESP32-C6/S3 Game Loop Manager using ESP-IDF
// Deterministic game loop manager optimized for ESP32 real-time performance
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include "../core/resource_manager.h"
#include "interface.h"
// #include "core.h"  // File not found, commenting out
#include "../graphics/engine.h"
#include <algorithm>  // For min/max functions
#include "../../utils/math/math.h"  // For Math::min/max functions

// Game state for level management
enum GameState {
    GAME_LOADING,       // Loading initial resources
    GAME_RUNNING,       // Normal gameplay
    GAME_STREAMING,     // Loading new chunk in background
    GAME_PAUSED,        // Game paused
    GAME_TRANSITIONING  // Moving between levels/areas
};

// Level loading strategy
enum LoadStrategy {
    LOAD_MINIMAL,       // Only current chunk
    LOAD_ADJACENT,      // Current + adjacent chunks
    LOAD_PREDICTIVE     // Load based on movement prediction
};

// Performance monitoring for adaptive loading
struct PerformanceMetrics {
    uint32_t frameTime;         // Last frame time in microseconds
    uint32_t avgFrameTime;      // Running average
    uint32_t loadingTime;       // Time spent loading resources
    uint32_t renderTime;        // Time spent rendering
    uint32_t logicTime;         // Time spent on game logic
    float fps;                  // Current FPS
    uint32_t memoryPressure;    // Current memory usage %
    
    // Frame timing history for adaptive loading
    uint32_t frameHistory[16];
    uint8_t frameHistoryIndex;
    
    PerformanceMetrics() : frameTime(0), avgFrameTime(16667), loadingTime(0),
                          renderTime(0), logicTime(0), fps(60.0f), 
                          memoryPressure(0), frameHistoryIndex(0) {
        memset(frameHistory, 0, sizeof(frameHistory));
    }
};

// Game loop manager with lazy loading integration
class GameLoopManager {
private:
    LazyResourceManager* resourceManager;
    WispEngine::Graphics::GraphicsEngine* graphics;
    WispAppBase* currentApp;
    
    GameState currentState;
    LoadStrategy loadStrategy;
    PerformanceMetrics metrics;
    
    // Timing control
    uint32_t targetFrameTime;   // Target frame time in microseconds (16667 = 60fps)
    uint32_t lastFrameStart;
    bool vsyncEnabled;
    
    // Level/chunk management
    uint16_t currentLevelId;
    static const int MAX_ACTIVE_CHUNKS = 16;
    uint16_t activeChunks[MAX_ACTIVE_CHUNKS];
    int activeChunkCount;
    int16_t lastPlayerX, lastPlayerY;
    uint16_t movementThreshold;  // Pixels before triggering new chunk loads
    
    // Streaming system
    bool backgroundStreamingEnabled;
    uint16_t streamingChunkId;
    bool isStreaming;
    
    // Adaptive loading
    bool adaptiveLoadingEnabled;
    uint32_t performanceBudget;  // Max microseconds per frame for loading
    
public:
    GameLoopManager(LazyResourceManager* resMgr, WispEngine::Graphics::GraphicsEngine* gfx) :
        resourceManager(resMgr), graphics(gfx), currentApp(nullptr),
        currentState(GAME_LOADING), loadStrategy(LOAD_ADJACENT),
        targetFrameTime(16667), lastFrameStart(0), vsyncEnabled(true),
        currentLevelId(0), lastPlayerX(0), lastPlayerY(0), movementThreshold(16),
        backgroundStreamingEnabled(true), streamingChunkId(0), isStreaming(false),
        adaptiveLoadingEnabled(true), performanceBudget(8000) {} // 8ms budget for loading
    
    // Main game loop - call this every frame
    void tick();
    
    // Level management
    bool loadLevel(uint16_t levelId, WispAppBase* app);
    void unloadCurrentLevel();
    bool isLevelLoaded() const { return currentState != GAME_LOADING; }
    
    // App management
    void setCurrentApp(WispAppBase* app) { currentApp = app; }
    WispAppBase* getCurrentApp() const { return currentApp; }
    
    // Chunk streaming control
    void updatePlayerPosition(int16_t x, int16_t y);
    void setLoadStrategy(LoadStrategy strategy) { loadStrategy = strategy; }
    void setMovementThreshold(uint16_t threshold) { movementThreshold = threshold; }
    
    // Performance control
    void setTargetFPS(float fps);
    void setVSync(bool enabled) { vsyncEnabled = enabled; }
    void setPerformanceBudget(uint32_t microseconds) { performanceBudget = microseconds; }
    void setAdaptiveLoading(bool enabled) { adaptiveLoadingEnabled = enabled; }
    
    // State control
    void pauseGame() { currentState = GAME_PAUSED; }
    void resumeGame() { currentState = GAME_RUNNING; }
    GameState getState() const { return currentState; }
    
    // Performance monitoring
    const PerformanceMetrics& getMetrics() const { return metrics; }
    float getCurrentFPS() const { return metrics.fps; }
    uint32_t getFrameTime() const { return metrics.frameTime; }
    bool isPerformanceGood() const;
    
    // Debug and monitoring
    void printPerformanceStats();
    void printChunkStatus();
    
private:
    // Core loop functions
    void processLoading();
    void processRunning();
    void processStreaming();
    void processTransitioning();
    
    // Streaming and loading
    void updateChunkLoading();
    void predictiveLoad();
    bool shouldLoadChunk(uint16_t chunkId);
    void backgroundStreamChunk();
    
    // Performance monitoring
    void updatePerformanceMetrics();
    void adaptLoadingBehavior();
    bool shouldReduceLoading() const;
    
    // Timing control
    void frameRateControl();
    void waitForVSync();
    
    // Memory management integration
    void handleMemoryPressure();
    void optimizeMemoryUsage();
};

// Implementation of core methods
inline void GameLoopManager::tick() {
    uint32_t frameStart = get_micros();
    
    // Update performance metrics from last frame
    if (lastFrameStart != 0) {
        metrics.frameTime = frameStart - lastFrameStart;
        updatePerformanceMetrics();
    }
    lastFrameStart = frameStart;
    
    // Handle different game states
    switch (currentState) {
        case GAME_LOADING:
            processLoading();
            break;
            
        case GAME_RUNNING:
            processRunning();
            break;
            
        case GAME_STREAMING:
            processStreaming();
            break;
            
        case GAME_TRANSITIONING:
            processTransitioning();
            break;
            
        case GAME_PAUSED:
            // Still render but don't update logic
            if (currentApp) {
                currentApp->render();
            }
            break;
    }
    
    // Adaptive loading behavior
    if (adaptiveLoadingEnabled) {
        adaptLoadingBehavior();
    }
    
    // Frame rate control
    frameRateControl();
}

inline void GameLoopManager::processRunning() {
    uint32_t logicStart = get_micros();
    
    // Run app logic
    if (currentApp) {
        currentApp->update();
    }
    
    metrics.logicTime = get_micros() - logicStart;
    
    // Check for chunk loading needs
    uint32_t loadingStart = get_micros();
    updateChunkLoading();
    
    // Background streaming if enabled and performance allows
    if (backgroundStreamingEnabled && !isStreaming && 
        get_micros() - loadingStart < performanceBudget / 2) {
        backgroundStreamChunk();
    }
    
    metrics.loadingTime = get_micros() - loadingStart;
    
    // Render
    uint32_t renderStart = get_micros();
    if (currentApp) {
        currentApp->render();
    }
    metrics.renderTime = get_micros() - renderStart;
    
    // Handle memory pressure
    if (resourceManager->getMemoryPressure() > 0.9f) {
        handleMemoryPressure();
    }
}

inline void GameLoopManager::updatePlayerPosition(int16_t x, int16_t y) {
    // Check if player moved enough to trigger chunk updates
    int16_t deltaX = abs(x - lastPlayerX);
    int16_t deltaY = abs(y - lastPlayerY);
    
    if (deltaX > movementThreshold || deltaY > movementThreshold) {
        lastPlayerX = x;
        lastPlayerY = y;
        
        // Update resource manager
        resourceManager->updatePlayerPosition(x, y);
        
        // Trigger predictive loading if enabled
        if (loadStrategy == LOAD_PREDICTIVE) {
            predictiveLoad();
        }
    }
}

inline void GameLoopManager::updateChunkLoading() {
    switch (loadStrategy) {
        case LOAD_MINIMAL:
            // Only load current chunk - resource manager handles this
            break;
            
        case LOAD_ADJACENT:
            // Load current + 8 adjacent chunks
            // Resource manager's proximity loading handles this
            break;
            
        case LOAD_PREDICTIVE:
            // Load based on movement direction and speed
            predictiveLoad();
            break;
    }
}

inline void GameLoopManager::predictiveLoad() {
    // TODO: Implement movement prediction
    // - Track player velocity
    // - Predict where player will be in next few seconds
    // - Preload chunks in that direction
    // - Consider player behavior patterns
}

inline bool GameLoopManager::loadLevel(uint16_t levelId, WispAppBase* app) {
    if (!resourceManager || !app) {
        return false;
    }
    
    currentState = GAME_LOADING;
    currentLevelId = levelId;
    currentApp = app;
    
    // Initialize the app
    bool success = app->init();
    if (!success) {
        WISP_DEBUG_ERROR("LOOP", "Failed to initialize app for level");
        return false;
    }
    
    // Load initial chunks around spawn point
    // App should tell us where the player starts
    int16_t spawnX = 0, spawnY = 0; // App should provide this
    resourceManager->updatePlayerPosition(spawnX, spawnY);
    
    currentState = GAME_RUNNING;
    WISP_DEBUG_INFO("LOOP", "Level loaded and running");
    
    return true;
}

inline void GameLoopManager::updatePerformanceMetrics() {
    // Update frame history
    metrics.frameHistory[metrics.frameHistoryIndex] = metrics.frameTime;
    metrics.frameHistoryIndex = (metrics.frameHistoryIndex + 1) % 16;
    
    // Calculate running average
    uint32_t total = 0;
    for (int i = 0; i < 16; i++) {
        total += metrics.frameHistory[i];
    }
    metrics.avgFrameTime = total / 16;
    
    // Calculate FPS
    metrics.fps = 1000000.0f / metrics.avgFrameTime;
    
    // Update memory pressure
    metrics.memoryPressure = (resourceManager->getCurrentMemoryUsage() * 100) / 
                            resourceManager->getMaxMemoryUsage();
}

inline void GameLoopManager::adaptLoadingBehavior() {
    // If performance is suffering, reduce loading
    if (shouldReduceLoading()) {
        // Temporarily reduce performance budget
        performanceBudget = Math::max_int(2000, performanceBudget - 1000); // Min 2ms
        
        // Switch to minimal loading
        if (loadStrategy != LOAD_MINIMAL) {
            loadStrategy = LOAD_MINIMAL;
            WISP_DEBUG_INFO("LOOP", "ADAPTIVE: Switching to minimal loading due to performance");
        }
        
        // Disable background streaming
        backgroundStreamingEnabled = false;
        
    } else if (metrics.avgFrameTime < targetFrameTime * 0.8f) {
        // Performance is good, can increase loading
        performanceBudget = Math::min_int(12000, performanceBudget + 500); // Max 12ms
        
        // Re-enable features
        if (!backgroundStreamingEnabled) {
            backgroundStreamingEnabled = true;
            WISP_DEBUG_INFO("LOOP", "ADAPTIVE: Re-enabling background streaming");
        }
        
        // Upgrade loading strategy if performance is really good
        if (metrics.avgFrameTime < targetFrameTime * 0.6f && loadStrategy == LOAD_MINIMAL) {
            loadStrategy = LOAD_ADJACENT;
            WISP_DEBUG_INFO("LOOP", "ADAPTIVE: Upgrading to adjacent loading");
        }
    }
}

inline bool GameLoopManager::shouldReduceLoading() const {
    // Multiple indicators of performance problems
    return (metrics.avgFrameTime > targetFrameTime * 1.2f) ||  // 20% over target
           (metrics.memoryPressure > 85) ||                     // High memory pressure
           (metrics.loadingTime > performanceBudget);           // Loading taking too long
}

inline void GameLoopManager::printPerformanceStats() {
    WISP_DEBUG_INFO("LOOP", "=== Game Loop Performance ===");
    WISP_DEBUG_INFO("LOOP", "FPS and Frame Time Statistics");
    WISP_DEBUG_INFO("LOOP", "Logic, Render, and Loading Times");  
    WISP_DEBUG_INFO("LOOP", "Memory Pressure Information");
    WISP_DEBUG_INFO("LOOP", "Load Strategy and Performance Budget");
    WISP_DEBUG_INFO("LOOP", "============================");
}
