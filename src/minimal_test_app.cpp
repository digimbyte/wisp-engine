// minimal_test_app.cpp - Test application for minimal engine
#ifdef PLATFORM_C6

#include "engine/minimal/minimal_engine.h"
#include "engine/minimal/minimal_api_wrapper.h"

using namespace WispEngine::Minimal;

void app_main() {
    ESP_LOGI("MinimalTest", "Starting ESP32-C6 Minimal Engine Test");
    
    // Initialize minimal engine
    if (!Engine::init()) {
        ESP_LOGE("MinimalTest", "Failed to initialize minimal engine");
        return;
    }
    
    ESP_LOGI("MinimalTest", "Minimal engine initialized successfully");
    
    // Test basic graphics
    Engine::graphics().clear(0x0000);  // Black background
    Engine::graphics().fillRect(10, 10, 50, 30, 0xF800);  // Red rectangle
    Engine::graphics().drawText(70, 20, "ESP32-C6", 0x07E0);  // Green text
    
    // Test sprite system
    Engine::graphics().drawSprite(1, 100, 50, 2);  // Draw sprite ID 1 at (100,50) with 2x scale
    Engine::graphics().drawSprite(2, 150, 80, 1);  // Draw sprite ID 2 at (150,80) with 1x scale
    
    Engine::graphics().display();
    
    // Test audio
    Engine::audio().playBeep();
    
    // Main loop
    while (true) {
        Engine::update();
        
        // Test input and respond
        if (Engine::input().wasButtonJustPressed(0)) {
            Engine::audio().playTone(800, 200);
            Engine::graphics().fillRect(0, 0, 240, 20, 0x001F);  // Blue bar
            Engine::graphics().drawText(5, 5, "Button Pressed!", 0xFFFF);
            Engine::graphics().display();
        }
        
        vTaskDelay(pdMS_TO_TICKS(16));  // ~60 FPS
    }
}

#endif // PLATFORM_C6
