// engine_config.h
#pragma once

// Engine configuration settings for pure C++ implementation
struct EngineConfig {
    // Performance settings
    bool enablePerformanceMonitoring = true;
    uint32_t maxFrameTime = 20000; // 20ms = 50 FPS minimum
    uint32_t targetFrameTime = 16666; // 16.67ms = 60 FPS target
    
    // Memory limits
    uint32_t maxEntities = 256;
    uint32_t maxRegions = 128;
    
    // Graphics settings
    bool enableDepthTesting = true;
    uint8_t defaultDepth = 6;
    
    // Audio settings
    bool enableAudio = true;
    uint16_t audioSampleRate = 44100;
    uint8_t audioChannels = 16;
    
    // Development settings
    bool enableDebugOutput = true;
    bool enableProfiler = false;
};

// Global engine configuration
extern EngineConfig engineConfig;

// Configuration presets
namespace EnginePresets {
    // High performance preset - optimized for native C++
    inline EngineConfig HighPerformance() {
        EngineConfig config;
        config.targetFrameTime = 16666; // 60 FPS
        config.maxFrameTime = 16666; // Strict timing
        config.enablePerformanceMonitoring = true;
        config.enableDebugOutput = false;
        return config;
    }
    
    // Development preset - debug features enabled
    inline EngineConfig Development() {
        EngineConfig config;
        config.targetFrameTime = 16666; // 60 FPS
        config.maxFrameTime = 33333; // Allow 30 FPS drops
        config.enablePerformanceMonitoring = true;
        config.enableDebugOutput = true;
        config.enableProfiler = true;
        return config;
    }
    
    // Memory constrained preset - minimal resource usage
    inline EngineConfig MemoryConstrained() {
        EngineConfig config;
        config.maxEntities = 128;
        config.maxRegions = 64;
        config.enableDebugOutput = false;
        config.enableProfiler = false;
        return config;
    }
    
    // Balanced preset - good performance with flexibility
    inline EngineConfig Balanced() {
        EngineConfig config; // Uses defaults
        return config;
    }
}

// Apply configuration preset
inline void applyEnginePreset(const EngineConfig& preset) {
    engineConfig = preset;
}
