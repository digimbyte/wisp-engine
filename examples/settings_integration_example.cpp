// settings_integration_example.cpp - Example of using SettingsManager with system initialization
// This demonstrates how to use the integrated SettingsManager in a real Wisp Engine application

#include "system/system_init.h"
#include "system/settings_manager.h"
#include "esp_log.h"

static const char* TAG = "SettingsExample";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Starting Wisp Engine Settings Integration Example");
    
    // Initialize the complete Wisp Engine system including settings
    wisp_init_result_t result = wisp_system_setup();
    
    if (result != WISP_INIT_OK) {
        ESP_LOGE(TAG, "System initialization failed with code: %d", result);
        return;
    }
    
    ESP_LOGI(TAG, "System initialized successfully!");
    
    // Now we can use the SettingsManager which was initialized during system setup
    using namespace WispEngine::System;
    
    try {
        SettingsManager& settings = SettingsManager::getInstance();
        
        ESP_LOGI(TAG, "=== Current Settings ===");
        ESP_LOGI(TAG, "Device Name: %s", settings.getDeviceName().c_str());
        ESP_LOGI(TAG, "WiFi SSID: %s", settings.getWiFiSSID().c_str());
        ESP_LOGI(TAG, "WiFi Auto-connect: %s", settings.getWiFiAutoConnect() ? "Yes" : "No");
        ESP_LOGI(TAG, "Bluetooth Enabled: %s", settings.getBluetoothEnabled() ? "Yes" : "No");
        ESP_LOGI(TAG, "Screen Brightness: %d/255", settings.getScreenBrightness());
        ESP_LOGI(TAG, "Audio Volume: %d/255", settings.getVolumeLevel());
        ESP_LOGI(TAG, "Hotspot Enabled: %s", settings.getHotspotEnabled() ? "Yes" : "No");
        ESP_LOGI(TAG, "Auto Sleep: %s (%d min)", 
                 settings.getAutoSleepEnabled() ? "Yes" : "No", 
                 settings.getAutoSleepMinutes());
        
        // Example: Update some settings
        ESP_LOGI(TAG, "\n=== Updating Settings ===");
        
        // Set a new device name
        settings.setDeviceName("my-wisp-device");
        ESP_LOGI(TAG, "Updated device name to: %s", settings.getDeviceName().c_str());
        
        // Configure WiFi settings
        settings.setWiFiSSID("MyWiFiNetwork");
        settings.setWiFiPassword("MySecurePassword");
        settings.setWiFiAutoConnect(true);
        ESP_LOGI(TAG, "Updated WiFi settings");
        
        // Configure display settings
        settings.setScreenBrightness(200); // 78% brightness
        ESP_LOGI(TAG, "Updated screen brightness to: %d/255", settings.getScreenBrightness());
        
        // Configure audio settings
        settings.setVolumeLevel(180); // 70% volume
        ESP_LOGI(TAG, "Updated volume to: %d/255", settings.getVolumeLevel());
        
        // Enable Bluetooth
        settings.setBluetoothEnabled(true);
        settings.setBluetoothDeviceName("Wisp-BT-Audio");
        ESP_LOGI(TAG, "Enabled Bluetooth with name: %s", settings.getBluetoothDeviceName().c_str());
        
        // Configure hotspot
        settings.setHotspotEnabled(true);
        settings.setHotspotName("WispEngine-AP");
        settings.setHotspotPassword("wisp123456");
        ESP_LOGI(TAG, "Configured hotspot: %s", settings.getHotspotName().c_str());
        
        // Save all changes to persistent storage
        SettingsError save_result = settings.saveSettings();
        if (save_result == SettingsError::SUCCESS) {
            ESP_LOGI(TAG, "✓ Settings saved successfully!");
        } else {
            ESP_LOGE(TAG, "✗ Failed to save settings: %s", 
                     settings.getErrorString(save_result).c_str());
        }
        
        // Export settings to a string (useful for debugging or backup)
        std::string settings_export;
        if (settings.exportSettings(settings_export) == SettingsError::SUCCESS) {
            ESP_LOGI(TAG, "\n=== Exported Settings ===\n%s", settings_export.c_str());
        }
        
        // Display storage information
        ESP_LOGI(TAG, "\n=== Storage Information ===");
        ESP_LOGI(TAG, "Using storage backend: %s", settings.isUsingNVS() ? "NVS" : "SPIFFS");
        ESP_LOGI(TAG, "Flash read-only: %s", settings.isFlashReadOnly() ? "Yes" : "No");
        ESP_LOGI(TAG, "Available space: %d bytes", settings.getAvailableSpace());
        
        // Run system diagnostics (includes settings test)
        ESP_LOGI(TAG, "\n=== Running System Diagnostics ===");
        bool diagnostics_passed = wisp_run_diagnostics();
        ESP_LOGI(TAG, "Diagnostics result: %s", diagnostics_passed ? "PASSED" : "FAILED");
        
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "Exception while using settings: %s", e.what());
    }
    
    // Main application loop
    ESP_LOGI(TAG, "\n=== Starting Main Loop ===");
    while (true) {
        // Call the system loop to handle all components
        wisp_system_loop();
        
        // Your application logic here
        
        // Example: Periodically save settings if needed
        static uint32_t last_save = 0;
        uint32_t now = get_millis();
        if (now - last_save > 30000) { // Save every 30 seconds if changes were made
            // In a real application, you'd track if changes were made
            // and only save when necessary
            last_save = now;
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay
    }
}

// Example function: How to update settings from a user interface
void example_update_settings_from_ui(const char* new_wifi_ssid, const char* new_wifi_password, 
                                     uint8_t new_brightness, uint8_t new_volume) {
    using namespace WispEngine::System;
    
    try {
        SettingsManager& settings = SettingsManager::getInstance();
        
        // Update settings
        if (new_wifi_ssid) {
            settings.setWiFiSSID(new_wifi_ssid);
        }
        if (new_wifi_password) {
            settings.setWiFiPassword(new_wifi_password);
        }
        settings.setScreenBrightness(new_brightness);
        settings.setVolumeLevel(new_volume);
        
        // Apply brightness setting immediately to hardware
        if (wisp_is_component_ready(WISP_COMPONENT_LCD)) {
            uint8_t brightness_percent = (new_brightness * 100) / 255;
            wisp_backlight_set(brightness_percent);
        }
        
        // Save changes
        SettingsError result = settings.saveSettings();
        if (result == SettingsError::SUCCESS) {
            ESP_LOGI(TAG, "Settings updated and saved successfully");
        } else {
            ESP_LOGE(TAG, "Failed to save settings: %s", 
                     settings.getErrorString(result).c_str());
        }
        
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "Exception updating settings: %s", e.what());
    }
}

// Example function: How to restore default settings
void example_restore_defaults() {
    using namespace WispEngine::System;
    
    try {
        SettingsManager& settings = SettingsManager::getInstance();
        
        SettingsError result = settings.resetToDefaults();
        if (result == SettingsError::SUCCESS) {
            ESP_LOGI(TAG, "Settings restored to defaults successfully");
            
            // Reapply default settings to hardware
            if (wisp_is_component_ready(WISP_COMPONENT_LCD)) {
                uint8_t brightness = settings.getScreenBrightness();
                uint8_t brightness_percent = (brightness * 100) / 255;
                wisp_backlight_set(brightness_percent);
            }
            
        } else {
            ESP_LOGE(TAG, "Failed to restore defaults: %s", 
                     settings.getErrorString(result).c_str());
        }
        
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "Exception restoring defaults: %s", e.what());
    }
}
