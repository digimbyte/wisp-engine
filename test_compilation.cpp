// test_compilation.cpp - Test core system compilation
// This file tests if the core bootloader and essential systems compile

#include "src/system/esp32_common.h"
#include "src/system/input_controller.h"
#include "src/engine/namespaces.h"

// Test function to check basic functionality
int main() {
    // Test ESP-IDF logging
    ESP_LOGI("TEST", "Testing ESP-IDF compilation");
    
    // Test timing functions
    uint32_t start_time = millis();
    uint64_t micro_time = micros();
    
    // Test input controller (should use ESP-IDF GPIO now)
    uint8_t test_pins[] = {4, 5, 6, 255};
    InputController input(test_pins);
    input.init();
    
    // Test namespace bridge
    Core::Timing::init();
    float fps = Core::Timing::getFPS();
    
    ESP_LOGI("TEST", "Compilation test successful - FPS: %.1f", fps);
    
    return 0;
}
