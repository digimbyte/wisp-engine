// test_compilation.cpp - Test core system compilation
// This file tests if the core bootloader and essential systems compile

#include "src/engine/engine_common.h"
#include "src/engine/wisp_engine_api.h"

// Test function to check unified engine compilation
int main() {
    // Test ESP-IDF logging
    ESP_LOGI("TEST", "Testing Wisp Engine compilation");
    
    // Test unified engine initialization
    if (!WispEngine::Engine::initialize()) {
        ESP_LOGE("TEST", "Failed to initialize engine");
        return -1;
    }
    
    // Test engine subsystem access
    auto* graphics = WispEngine::Engine::getGraphics();
    auto* database = WispEngine::Engine::getDatabase();
    
    ESP_LOGI("TEST", "Engine version: %s", WispEngine::Engine::getVersion());
    ESP_LOGI("TEST", "Compilation test successful");
    
    // Clean shutdown
    WispEngine::Engine::shutdown();
    
    return 0;
}
