// test_namespaces.cpp - Simple ESP-IDF test for namespace bridges
#include "engine/namespaces.h"
#include "system/esp32_common.h"

// Simple test functions to verify namespace bridges work
void testDebugNamespace() {
    WispEngine::Core::Debug::init(
        WispEngine::Core::Debug::DEBUG_MODE_ENABLED,
        WispEngine::Core::Debug::SAFETY_ENABLED
    );
    
    WispEngine::Core::Debug::info("TEST", "Debug system working via namespace bridge");
    WispEngine::Core::Debug::warning("TEST", "Warning system test");
    WispEngine::Core::Debug::error("TEST", "Error system test");
}

void testTimingNamespace() {
    WispEngine::Core::Timing::init();
    
    WISP_DEBUG_INFO("TEST", "Timing namespace initialized");
    
    // Test a few frames
    for (int i = 0; i < 10; i++) {
        if (WispEngine::Core::Timing::frameReady()) {
            WispEngine::Core::Timing::tick();
            WISP_DEBUG_INFO("TEST", "Frame timing available");
        }
        vTaskDelay(pdMS_TO_TICKS(16)); // ~60 FPS target
    }
}

void runNamespaceTests() {
    WISP_DEBUG_INFO("TEST", "Starting namespace bridge tests...");
    
    testDebugNamespace();
    testTimingNamespace();
    
    WISP_DEBUG_INFO("TEST", "All namespace bridge tests completed successfully!");
    WispEngine::Core::Debug::shutdown();
}
