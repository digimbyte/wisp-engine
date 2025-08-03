// engine/namespace_integration.h - Central integration for namespace bridges
#pragma once

// Include the bridge implementations
#include "../system/debug_esp32.h"
#include "core/timing.h" 
#include "graphics/namespace_bridge.h"
#include "audio/namespace_bridge.h"

// Main Engine namespace integration
namespace WispEngine {

class Engine {
private:
    static bool initialized;
    
public:
    static bool initialize() {
        if (initialized) return true;
        
        // Initialize all subsystems in proper order
        if (!Graphics::Engine::initialize()) {
            Core::Debug::error("ENGINE", "Graphics initialization failed");
            return false;
        }
        
        if (!Audio::Engine::initialize()) {
            Core::Debug::error("ENGINE", "Audio initialization failed");
            return false;
        }
        
        Core::Timing::init();
        Core::Debug::info("ENGINE", "Wisp Engine initialized successfully");
        
        initialized = true;
        return true;
    }
    
    static void shutdown() {
        if (!initialized) return;
        
        Core::Debug::info("ENGINE", "Shutting down Wisp Engine");
        
        Audio::Engine::shutdown();
        Graphics::Engine::shutdown();
        Core::Debug::shutdown();
        
        initialized = false;
    }
    
    static Graphics::Engine* getGraphics() {
        return Graphics::Engine::getInstance();
    }
    
    static Audio::Engine* getAudio() {
        return Audio::Engine::getInstance();
    }
    
    static bool isInitialized() {
        return initialized;
    }
    
    static const char* getVersion() {
        return WISP_ENGINE_VERSION;
    }
};

// Static member definition
bool Engine::initialized = false;

} // namespace WispEngine
