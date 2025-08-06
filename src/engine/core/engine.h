// engine/core.h - ESP32-C6/S3 Core Engine using ESP-IDF
// Main engine core with timing, entities, and system management for ESP32
#pragma once
#include "../engine_common.h"
#include <stdint.h>
#include "timing.h"
#include "entity_system.h"
#include "physics_system.h"
#include "render_system.h"
#include "audio_system.h"

// Core engine states
enum EngineState {
    ENGINE_UNINITIALIZED,
    ENGINE_INITIALIZING,
    ENGINE_RUNNING,
    ENGINE_PAUSED,
    ENGINE_SHUTTING_DOWN,
    ENGINE_ERROR
};

// Core engine subsystems - clean separation of concerns
class EngineCore {
private:
    EngineState currentState;
    TimingSystem* timing;
    EntitySystem* entities;
    PhysicsSystem* physics;
    RenderSystem* renderer;
    AudioSystem* audio;
    
    // Performance tracking
    uint32_t frameCount;
    uint32_t lastFrameTime;
    uint32_t targetFrameTime;
    
public:
    EngineCore() : 
        currentState(ENGINE_UNINITIALIZED),
        timing(nullptr),
        entities(nullptr),
        physics(nullptr),
        renderer(nullptr),
        audio(nullptr),
        frameCount(0),
        lastFrameTime(0),
        targetFrameTime(16666) {} // 60 FPS default
    
    // Core lifecycle
    bool initialize();
    bool shutdown();
    void update();
    void pause();
    void resume();
    
    // Frame timing
    void setTargetFrameRate(uint8_t fps);
    bool isFrameReady();
    void beginFrame();
    void endFrame();
    
    // System access
    TimingSystem* getTiming() const { return timing; }
    EntitySystem* getEntities() const { return entities; }
    PhysicsSystem* getPhysics() const { return physics; }
    RenderSystem* getRenderer() const { return renderer; }
    AudioSystem* getAudio() const { return audio; }
    
    // State management
    EngineState getState() const { return currentState; }
    bool isRunning() const { return currentState == ENGINE_RUNNING; }
    
    // Performance metrics
    uint32_t getFrameCount() const { return frameCount; }
    uint32_t getFrameTime() const { return lastFrameTime; }
    uint32_t getTargetFrameTime() const { return targetFrameTime; }
    
    // Debug and diagnostics
    void printSystemStatus();
    bool validateSystems();
    
private:
    bool initializeSystems();
    void shutdownSystems();
    void updateSystems();
    
    void setState(EngineState newState);
    void handleStateTransition(EngineState oldState, EngineState newState);
};
