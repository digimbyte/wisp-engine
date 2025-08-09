# SettingsManager Integration Guide

The Wisp Engine now includes a fully integrated `SettingsManager` that provides persistent storage of all engine configuration settings. This document explains how to use the integrated settings system.

## Overview

The `SettingsManager` is automatically initialized as part of the system boot process and provides:

- **Persistent Storage**: Uses NVS (Non-Volatile Storage) on ESP32 platforms with SPIFFS fallback
- **Comprehensive Settings**: WiFi, Bluetooth, hotspot, system, and audio configuration
- **Automatic Integration**: Settings are applied to hardware during system initialization
- **Error Handling**: Comprehensive error reporting and recovery
- **Configuration Export**: Human-readable INI-style configuration export

## Quick Start

### 1. System Initialization

The SettingsManager is automatically initialized when you call the system setup:

```cpp
#include "system/system_init.h"

extern "C" void app_main(void) {
    // Initialize all system components including settings
    wisp_init_result_t result = wisp_system_setup();
    
    if (result != WISP_INIT_OK) {
        ESP_LOGE(TAG, "System initialization failed");
        return;
    }
    
    // Settings are now loaded and ready to use
}
```

### 2. Using Settings

```cpp
#include "system/settings_manager.h"

using namespace WispEngine::System;

// Get the singleton instance
SettingsManager& settings = SettingsManager::getInstance();

// Read settings
std::string device_name = settings.getDeviceName();
uint8_t brightness = settings.getScreenBrightness();
bool wifi_enabled = settings.getWiFiAutoConnect();

// Modify settings
settings.setDeviceName("my-device");
settings.setScreenBrightness(200);
settings.setWiFiAutoConnect(true);

// Save changes to persistent storage
SettingsError result = settings.saveSettings();
if (result != SettingsError::SUCCESS) {
    ESP_LOGE(TAG, "Failed to save: %s", settings.getErrorString(result).c_str());
}
```

## Available Settings

### Network Settings
- `getWiFiSSID()` / `setWiFiSSID()`
- `getWiFiPassword()` / `setWiFiPassword()`
- `getWiFiAutoConnect()` / `setWiFiAutoConnect()`
- `getWiFiPower()` / `setWiFiPower()`

### Bluetooth Settings
- `getBluetoothEnabled()` / `setBluetoothEnabled()`
- `getBluetoothAudioStreaming()` / `setBluetoothAudioStreaming()`
- `getBluetoothDeviceName()` / `setBluetoothDeviceName()`

### Hotspot Settings
- `getHotspotEnabled()` / `setHotspotEnabled()`
- `getHotspotName()` / `setHotspotName()`
- `getHotspotPassword()` / `setHotspotPassword()`

### System Settings
- `getDeviceName()` / `setDeviceName()`
- `getMDNSEnabled()` / `setMDNSEnabled()`
- `getScreenBrightness()` / `setScreenBrightness()`
- `getAutoSleepEnabled()` / `setAutoSleepEnabled()`
- `getAutoSleepMinutes()` / `setAutoSleepMinutes()`

### Audio Settings
- `getVolumeLevel()` / `setVolumeLevel()`
- `getAudioEnabled()` / `setAudioEnabled()`

## System Integration

The SettingsManager is integrated with the hardware components:

### Screen Brightness
When the LCD component is initialized, the settings manager automatically applies the saved brightness setting:

```cpp
// This happens automatically during system init
if (g_system_status.lcd_ready) {
    uint8_t brightness = settings.getScreenBrightness();
    uint8_t brightness_percent = (brightness * 100) / 255;
    wisp_backlight_set(brightness_percent);
}
```

### Settings Status
The settings component status is included in system diagnostics:

```cpp
// Check if settings are ready
if (wisp_is_component_ready(WISP_COMPONENT_SETTINGS)) {
    ESP_LOGI(TAG, "Settings manager is ready");
}

// Run full system diagnostics including settings test
wisp_run_diagnostics();
```

## Storage Backends

### Primary: NVS (Non-Volatile Storage)
- Default on ESP32 platforms
- Fast access times
- Atomic operations
- Wear leveling built-in

### Fallback: SPIFFS
- Used if NVS initialization fails
- File-based storage (`/spiffs/settings.ini`)
- Human-readable format
- Can be manually edited if needed

## Error Handling

The system provides comprehensive error handling:

```cpp
SettingsError result = settings.saveSettings();
switch (result) {
    case SettingsError::SUCCESS:
        ESP_LOGI(TAG, "Settings saved successfully");
        break;
    case SettingsError::FLASH_READ_ONLY:
        ESP_LOGW(TAG, "Flash is read-only, cannot save");
        break;
    case SettingsError::OUT_OF_SPACE:
        ESP_LOGE(TAG, "Insufficient storage space");
        break;
    default:
        ESP_LOGE(TAG, "Save failed: %s", settings.getErrorString(result).c_str());
        break;
}
```

## Configuration Export/Import

Export current settings to a human-readable format:

```cpp
std::string config_string;
if (settings.exportSettings(config_string) == SettingsError::SUCCESS) {
    ESP_LOGI(TAG, "Current configuration:\n%s", config_string.c_str());
}
```

Example exported configuration:
```ini
# Wisp Engine Settings Configuration
# Generated automatically - do not edit manually

[Network]
wifi_ssid=MyWiFiNetwork
wifi_password=MySecurePassword
wifi_auto_connect=true
wifi_power=20

[Bluetooth]
bluetooth_enabled=true
bluetooth_audio=false
bluetooth_device_name=Wisp-BT-Audio

[Hotspot]
hotspot_enabled=false
hotspot_name=WispEngine
hotspot_password=wisp1234

[System]
device_name=my-wisp-device
mdns_enabled=true
screen_brightness=200
auto_sleep_enabled=true
auto_sleep_minutes=30

[Audio]
volume_level=128
audio_enabled=true
audio_sample_rate=44

system_version=1
```

## Best Practices

### 1. Check Component Status
Always verify that the settings component is ready before use:

```cpp
if (!wisp_is_component_ready(WISP_COMPONENT_SETTINGS)) {
    ESP_LOGW(TAG, "Settings not available");
    return;
}
```

### 2. Batch Updates
When updating multiple settings, batch them together and save once:

```cpp
settings.setWiFiSSID("newSSID");
settings.setWiFiPassword("newPassword");
settings.setWiFiAutoConnect(true);
// Save all changes at once
settings.saveSettings();
```

### 3. Apply Hardware Changes
When updating display or audio settings, apply them to hardware immediately:

```cpp
settings.setScreenBrightness(new_brightness);
if (wisp_is_component_ready(WISP_COMPONENT_LCD)) {
    uint8_t percent = (new_brightness * 100) / 255;
    wisp_backlight_set(percent);
}
settings.saveSettings();
```

### 4. Error Recovery
Handle storage errors gracefully:

```cpp
SettingsError result = settings.saveSettings();
if (result != SettingsError::SUCCESS) {
    // Log the error but continue operation
    ESP_LOGW(TAG, "Could not save settings: %s", 
             settings.getErrorString(result).c_str());
    
    // Maybe try again later or notify user
    if (result == SettingsError::OUT_OF_SPACE) {
        // Handle storage full condition
    }
}
```

## Troubleshooting

### Settings Not Saving
1. Check if flash is read-only: `settings.isFlashReadOnly()`
2. Verify available space: `settings.getAvailableSpace()`
3. Check error codes from `saveSettings()`

### Settings Not Loading
1. Verify system initialization completed successfully
2. Check system diagnostics: `wisp_run_diagnostics()`
3. Look for initialization errors in logs

### Storage Issues
1. NVS initialization failure falls back to SPIFFS automatically
2. Check flash chip detection in system logs
3. Verify partition table includes NVS and SPIFFS partitions

## Integration with User Interfaces

When building user interfaces (LVGL menus, web interfaces, etc.), use the SettingsManager as the single source of truth:

```cpp
// In menu update callback
void on_brightness_changed(uint8_t new_brightness) {
    SettingsManager& settings = SettingsManager::getInstance();
    
    settings.setScreenBrightness(new_brightness);
    
    // Apply immediately to hardware
    if (wisp_is_component_ready(WISP_COMPONENT_LCD)) {
        wisp_backlight_set((new_brightness * 100) / 255);
    }
    
    // Save for persistence
    settings.saveSettings();
}
```

The integrated SettingsManager provides a robust, easy-to-use configuration system that handles all the complexity of persistent storage while providing a clean API for your application code.
