// wisp_main_demo.cpp - Complete Wisp Engine System Demo
// This demonstrates the ESP-IDF library pattern you requested

#include "system/system_init.h"
#include "system/led_controller.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "WispMainDemo";

extern "C" void app_main(void);

/**
 * @brief Complete system setup following your ESP-IDF pattern
 * Equivalent to the setup() function you described
 */
bool setup() {
    ESP_LOGI(TAG, "=== Wisp Engine System Setup ===");
    
    // Initialize the complete Wisp Engine system
    wisp_init_result_t result = wisp_system_setup();
    
    if (result != WISP_INIT_OK) {
        ESP_LOGE(TAG, "System initialization failed with error: %d", result);
        return false;
    }
    
    // Individual component initialization (as per your list):
    
    // 1. Wireless_Init() - Initialize wireless communication
    if (wisp_is_component_ready(WISP_COMPONENT_WIRELESS)) {
        ESP_LOGI(TAG, "✓ Wireless communication initialized");
    } else {
        ESP_LOGW(TAG, "○ Wireless not available");
    }
    
    // 2. Flash_Searching() - Test and print flash memory info
    const wisp_system_status_t* status = wisp_get_system_status();
    if (status->flash_ready) {
        ESP_LOGI(TAG, "✓ Flash memory: %d MB detected", status->flash_size_mb);
    }
    
    // 3. RGB_Init() - Initialize RGB functions
    if (wisp_is_component_ready(WISP_COMPONENT_RGB)) {
        ESP_LOGI(TAG, "✓ RGB LEDs initialized");
        
        // 4. RGB_Example() - Display RGB examples
        wisp_rgb_example();
    }
    
    // 5. SD_Init() - Initialize TF card
    if (wisp_is_component_ready(WISP_COMPONENT_SD)) {
        ESP_LOGI(TAG, "✓ SD/TF card initialized");
    }
    
    // 6. LCD_Init() - Initialize display
    if (wisp_is_component_ready(WISP_COMPONENT_LCD)) {
        ESP_LOGI(TAG, "✓ LCD display initialized");
        
        // 7. BK_Light(50) - Set backlight brightness to 50%
        wisp_backlight_set(50);
        ESP_LOGI(TAG, "✓ Backlight set to 50%%");
    }
    
    // 8. LVGL_Init() - Initialize LVGL graphics library
    if (wisp_is_component_ready(WISP_COMPONENT_LVGL)) {
        ESP_LOGI(TAG, "✓ LVGL graphics library initialized");
        
        // 9. Lvgl_Example1() - Run LVGL example
        wisp_lvgl_example1();
    }
    
    // Run system diagnostics
    wisp_run_diagnostics();
    
    ESP_LOGI(TAG, "=== System Setup Complete ===");
    return true;
}

/**
 * @brief Main application loop following your ESP-IDF pattern
 * Equivalent to the while(1) loop you described
 */
void main_loop() {
    ESP_LOGI(TAG, "Starting main application loop...");
    
    uint32_t loop_count = 0;
    uint32_t last_status_time = 0;
    const uint32_t STATUS_INTERVAL_MS = 10000; // Print status every 10 seconds
    
    while (1) {
        // Update the system (LED animations, etc.)
        wisp_system_loop();
        
        // vTaskDelay(pdMS_TO_TICKS(10)) - Short delay every 10ms
        wisp_delay_ms(10);
        
        // lv_timer_handler() - LVGL timer handling (called by wisp_system_loop)
        // This is already handled in wisp_system_loop(), but we could call it directly:
        // wisp_lvgl_timer_handler();
        
        loop_count++;
        
        // Periodic status update
        uint32_t current_time = get_millis();
        if (current_time - last_status_time >= STATUS_INTERVAL_MS) {
            ESP_LOGI(TAG, "Main loop running: %d iterations, uptime: %d ms", 
                     loop_count, current_time);
            
            // Optional: Print system status periodically
            if (loop_count % 100 == 0) {
                wisp_print_system_status();
            }
            
            last_status_time = current_time;
        }
        
        // Example of using LED status indicators based on system state
        static uint32_t last_led_update = 0;
        if (current_time - last_led_update >= 5000) { // Every 5 seconds
            if (wisp_is_component_ready(WISP_COMPONENT_RGB)) {
                // Cycle through status indicators
                static int status_cycle = 0;
                switch (status_cycle % 4) {
                    case 0:
                        LED_SUCCESS(); // Green - system OK
                        break;
                    case 1:
                        LED_INFO();    // Blue - information
                        break;
                    case 2:
                        if (!wisp_is_component_ready(WISP_COMPONENT_SD)) {
                            LED_WARNING(); // Orange - SD card missing
                        }
                        break;
                    case 3:
                        LED_PULSE(128, 128, 255, 1000); // Purple pulse
                        break;
                }
                status_cycle++;
            }
            last_led_update = current_time;
        }
    }
}

/**
 * @brief FreeRTOS task wrapper for the main loop
 */
void main_loop_task(void* pvParameters) {
    main_loop(); // This runs the infinite loop
    
    // Should never reach here, but clean up if we do
    ESP_LOGE(TAG, "Main loop exited unexpectedly!");
    vTaskDelete(NULL);
}

/**
 * @brief ESP-IDF application entry point
 * This follows the exact pattern you described in your ESP-IDF library
 */
void app_main(void) {
    ESP_LOGI(TAG, "Wisp Engine Demo Starting...");
    ESP_LOGI(TAG, "ESP-IDF Version: %s", esp_get_idf_version());
    
    // Perform system setup
    if (!setup()) {
        ESP_LOGE(TAG, "System setup failed - halting");
        LED_ERROR();
        while (1) {
            wisp_delay_ms(1000);
        }
    }
    
    // Show boot sequence on LEDs
    if (wisp_is_component_ready(WISP_COMPONENT_RGB)) {
        ledController.showBootSequence();
    }
    
    // Create the main loop task
    BaseType_t result = xTaskCreate(
        main_loop_task,           // Task function
        "wisp_main_loop",         // Task name
        4096,                     // Stack size (bytes)
        NULL,                     // Parameters
        5,                        // Priority
        NULL                      // Task handle
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create main loop task");
        LED_ERROR();
        while (1) {
            wisp_delay_ms(1000);
        }
    }
    
    ESP_LOGI(TAG, "Main loop task created successfully");
    
    // The FreeRTOS scheduler will now handle task execution
    // app_main() can return since the main loop runs in its own task
}

/**
 * @brief Alternative simple main for testing
 * This is a simpler version without FreeRTOS tasks
 */
void simple_main(void) {
    ESP_LOGI(TAG, "Simple Wisp Engine Demo");
    
    // System setup
    if (!setup()) {
        ESP_LOGE(TAG, "Setup failed!");
        return;
    }
    
    // Boot sequence
    if (wisp_is_component_ready(WISP_COMPONENT_RGB)) {
        ledController.showBootSequence();
    }
    
    // Simple loop
    uint32_t count = 0;
    while (1) {
        wisp_system_loop();
        
        if (count % 1000 == 0) {
            ESP_LOGI(TAG, "Loop iteration: %d", count);
        }
        
        wisp_delay_ms(10);
        count++;
    }
}

// Demonstration functions for specific features

/**
 * @brief Demonstrate LED controller features
 */
void demo_led_features(void) {
    if (!wisp_is_component_ready(WISP_COMPONENT_RGB)) {
        ESP_LOGW(TAG, "RGB LEDs not available for demo");
        return;
    }
    
    ESP_LOGI(TAG, "=== LED Features Demo ===");
    
    // Basic colors
    ESP_LOGI(TAG, "Basic colors...");
    LED_SET_COLOR(255, 0, 0); LED_SHOW(); wisp_delay_ms(500);
    LED_SET_COLOR(0, 255, 0); LED_SHOW(); wisp_delay_ms(500);
    LED_SET_COLOR(0, 0, 255); LED_SHOW(); wisp_delay_ms(500);
    
    // Brightness control
    ESP_LOGI(TAG, "Brightness control...");
    LED_SET_COLOR(255, 255, 255);
    for (int brightness = 0; brightness <= 100; brightness += 10) {
        LED_BRIGHTNESS(brightness / 100.0f);
        LED_SHOW();
        wisp_delay_ms(100);
    }
    
    // Animations
    ESP_LOGI(TAG, "Animations...");
    LED_RAINBOW(3000);
    wisp_delay_ms(3000);
    
    LED_PULSE(255, 0, 255, 2000);
    wisp_delay_ms(4000);
    
    // Status indicators
    ESP_LOGI(TAG, "Status indicators...");
    LED_SUCCESS();
    wisp_delay_ms(1000);
    LED_WARNING();
    wisp_delay_ms(1500);
    LED_ERROR();
    wisp_delay_ms(2000);
    
    LED_CLEAR();
    LED_SHOW();
    
    ESP_LOGI(TAG, "LED demo complete");
}

/**
 * @brief Demonstrate system monitoring
 */
void demo_system_monitoring(void) {
    ESP_LOGI(TAG, "=== System Monitoring Demo ===");
    
    const wisp_system_status_t* status = wisp_get_system_status();
    
    ESP_LOGI(TAG, "System Status:");
    ESP_LOGI(TAG, "  Wireless: %s", status->wireless_ready ? "Ready" : "Not Ready");
    ESP_LOGI(TAG, "  Flash: %s (%d MB)", status->flash_ready ? "Ready" : "Not Ready", status->flash_size_mb);
    ESP_LOGI(TAG, "  RGB LEDs: %s", status->rgb_ready ? "Ready" : "Not Ready");
    ESP_LOGI(TAG, "  SD Card: %s", status->sd_ready ? "Ready" : "Not Ready");
    ESP_LOGI(TAG, "  LCD: %s", status->lcd_ready ? "Ready" : "Not Ready");
    ESP_LOGI(TAG, "  LVGL: %s", status->lvgl_ready ? "Ready" : "Not Ready");
    ESP_LOGI(TAG, "  Backlight: %d%%", status->backlight_level);
    ESP_LOGI(TAG, "  Init Time: %d ms", status->init_time_ms);
    
    // Test individual components
    ESP_LOGI(TAG, "Component Status:");
    ESP_LOGI(TAG, "  WIRELESS: %s", wisp_get_component_status_string(WISP_COMPONENT_WIRELESS));
    ESP_LOGI(TAG, "  FLASH:    %s", wisp_get_component_status_string(WISP_COMPONENT_FLASH));
    ESP_LOGI(TAG, "  RGB:      %s", wisp_get_component_status_string(WISP_COMPONENT_RGB));
    ESP_LOGI(TAG, "  SD:       %s", wisp_get_component_status_string(WISP_COMPONENT_SD));
    ESP_LOGI(TAG, "  LCD:      %s", wisp_get_component_status_string(WISP_COMPONENT_LCD));
    ESP_LOGI(TAG, "  LVGL:     %s", wisp_get_component_status_string(WISP_COMPONENT_LVGL));
}
