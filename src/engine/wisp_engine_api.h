// wisp_engine_api.h - Unified Wisp Engine Interface
#pragma once
#include "engine_common.h"
#include "graphics/engine.h"
#include "audio/audio_engine.h"
#include "database/partitioned_system.h"
#include "core/engine.h"

namespace WispEngine {

/**
 * Unified Engine Interface - Single point of access to all engine subsystems
 */
class Engine {
public:
    /**
     * Initialize the entire engine with default settings
     */
    static bool initialize() {
        // Initialize core systems first
        if (!initializeCore()) {
            return false;
        }
        
        // Initialize audio system
        Audio::init();
        
        // Initialize graphics system
        if (!initializeGraphics()) {
            Audio::shutdown();
            return false;
        }
        
        // Initialize database system
        if (!initializeDatabase()) {
            Audio::shutdown();
            shutdownGraphics();
            return false;
        }
        
        initialized = true;
        return true;
    }
    
    /**
     * Shutdown the entire engine
     */
    static void shutdown() {
        if (!initialized) return;
        
        Audio::shutdown();
        shutdownGraphics();
        shutdownDatabase();
        shutdownCore();
        
        initialized = false;
    }
    
    /**
     * Update all engine systems - call once per frame
     */
    static void update() {
        if (!initialized) return;
        
        Audio::update();
        // Graphics and database updates are typically handled internally
    }
    
    // === SUBSYSTEM ACCESS ===
    
    /**
     * Get graphics engine instance
     */
    static GraphicsEngine* getGraphics() {
        return graphicsEngine;
    }
    
    /**
     * Get database system instance
     */
    static WispPartitionedDB* getDatabase() {
        return databaseEngine;
    }
    
    /**
     * Check if engine is initialized
     */
    static bool isInitialized() {
        return initialized;
    }
    
    /**
     * Get engine version
     */
    static const char* getVersion() {
        return WISP_ENGINE_VERSION;
    }

private:
    static bool initialized;
    static GraphicsEngine* graphicsEngine;
    static WispPartitionedDB* databaseEngine;
    
    static bool initializeCore() {
        // Core system initialization
        return true;
    }
    
    static bool initializeGraphics() {
        // Initialize graphics engine
        if (!graphicsEngine) {
            graphicsEngine = new GraphicsEngine();
            return graphicsEngine->init();
        }
        return true;
    }
    
    static bool initializeDatabase() {
        // Initialize database system
        if (!databaseEngine) {
            databaseEngine = new WispPartitionedDB();
            return databaseEngine->init();
        }
        return true;
    }
    
    static void shutdownCore() {
        // Core system cleanup
    }
    
    static void shutdownGraphics() {
        if (graphicsEngine) {
            delete graphicsEngine;
            graphicsEngine = nullptr;
        }
    }
    
    static void shutdownDatabase() {
        if (databaseEngine) {
            delete databaseEngine;
            databaseEngine = nullptr;
        }
    }
};

// Static member declarations
bool Engine::initialized = false;
GraphicsEngine* Engine::graphicsEngine = nullptr;
WispPartitionedDB* Engine::databaseEngine = nullptr;

} // namespace WispEngine

// === CONVENIENCE MACROS ===
#undef WISP_ENGINE_INIT
#undef WISP_ENGINE_SHUTDOWN
#define WISP_ENGINE_INIT() WispEngine::Engine::initialize()
#define WISP_ENGINE_SHUTDOWN() WispEngine::Engine::shutdown()
