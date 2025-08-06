# Wisp Engine Feature Flag System

The Wisp Engine implements a comprehensive feature flag system that allows developers to enable or disable WiFi and Bluetooth functionality based on the specific ESP32 board variant being used.

## Overview

Different ESP32 modules have varying capabilities:
- **ESP32-C6**: Full WiFi 6 + Bluetooth 5.0 LE support
- **ESP32-S3-WROOM**: WiFi + Bluetooth support (wireless modules)
- **ESP32-S3 Basic**: No wireless capabilities (basic modules)

The feature flag system ensures that:
1. Code only compiles features available on the target hardware
2. Menu systems automatically adapt to available features
3. Settings persistence works correctly regardless of available features
4. Memory usage is optimized by excluding unused code

## Feature Flags

### Core Feature Flags

| Flag | Description | Default C6 | Default S3 |
|------|-------------|------------|------------|
| `WISP_HAS_WIFI` | WiFi networking support | 1 | 0 |
| `WISP_HAS_BLUETOOTH` | Bluetooth LE support | 1 | 0 |
| `WISP_HAS_BLUETOOTH_CLASSIC` | Bluetooth Classic support | 1 | 0 |
| `WISP_HAS_WIFI_DIRECT` | WiFi Direct/P2P support | 1 | 0 |
| `WISP_HAS_EXTERNAL_STORAGE` | SD card/external storage | 1 | 0 |

### Board Configuration

Feature flags are defined in board configuration headers:

**ESP32-C6** (`boards/esp32-c6_config.h`):
```cpp
#define WISP_HAS_WIFI               1    // WiFi 6 available
#define WISP_HAS_BLUETOOTH          1    // Bluetooth 5.0 LE available
#define WISP_HAS_BLUETOOTH_CLASSIC  1    // Bluetooth Classic available
#define WISP_HAS_WIFI_DIRECT        1    // WiFi Direct available
#define WISP_HAS_EXTERNAL_STORAGE   1    // TF card available
```

**ESP32-S3** (`boards/esp32-s3_config.h`):
```cpp
// Conditional compilation based on module variant
#ifndef WISP_HAS_WIFI
    #define WISP_HAS_WIFI           0    // Default: disabled
#endif
// ... (similar for other features)
```

## PlatformIO Configuration

Use build flags in `platformio.ini` to enable features for specific boards:

### ESP32-C6 (Full Features)
```ini
[env:esp32-c6-lcd]
platform = espressif32
board = esp32-c6-devkitm-1
build_flags = 
    -DPLATFORM_C6=1
    -DWISP_HAS_WIFI=1
    -DWISP_HAS_BLUETOOTH=1
    -DWISP_HAS_BLUETOOTH_CLASSIC=1
    -DWISP_HAS_WIFI_DIRECT=1
    -DWISP_HAS_EXTERNAL_STORAGE=1
```

### ESP32-S3 with WiFi/Bluetooth (WROOM modules)
```ini
[env:esp32-s3-round-wireless]
platform = espressif32
board = esp32-s3-devkitc-1
build_flags = 
    -DPLATFORM_S3=1
    -DWISP_HAS_WIFI=1
    -DWISP_HAS_BLUETOOTH=1
    -DWISP_HAS_BLUETOOTH_CLASSIC=1
    -DWISP_HAS_WIFI_DIRECT=1
    -DWISP_HAS_EXTERNAL_STORAGE=0
```

### ESP32-S3 Basic (No Wireless)
```ini
[env:esp32-s3-round-basic]
platform = espressif32
board = esp32-s3-devkitc-1
build_flags = 
    -DPLATFORM_S3=1
    -DWISP_HAS_WIFI=0
    -DWISP_HAS_BLUETOOTH=0
    -DWISP_HAS_BLUETOOTH_CLASSIC=0
    -DWISP_HAS_WIFI_DIRECT=0
    -DWISP_HAS_EXTERNAL_STORAGE=0
```

## Code Implementation

### Settings Class
The Settings class provides conditional methods based on feature availability:

```cpp
#if WISP_HAS_WIFI
  bool getWiFiEnabled() { /* real implementation */ }
  void setWiFiEnabled(bool enabled) { /* real implementation */ }
#else
  // Stub methods for boards without WiFi
  bool getWiFiEnabled() { return false; }
  void setWiFiEnabled(bool enabled) { /* No-op */ }
#endif
```

### Menu System
The menu system automatically adapts based on available features:

```cpp
enum SettingsOption {
    SETTINGS_THEME = 0,
#if WISP_HAS_WIFI
    SETTINGS_WIFI,
#endif
#if WISP_HAS_BLUETOOTH
    SETTINGS_BLUETOOTH,
#endif
    SETTINGS_PROFILE,
    SETTINGS_COUNT
};
```

### Panel Creation
Panels are only created if the corresponding feature is available:

```cpp
#if WISP_HAS_WIFI
    wifiSettings = new WiFiSettingsPanel();
    // ... initialization
#endif

#if WISP_HAS_BLUETOOTH
    bluetoothSettings = new BluetoothSettingsPanel();
    // ... initialization
#endif
```

## Benefits

### 1. Memory Optimization
- Unused code is completely excluded from compilation
- Smaller binary size for boards with limited features
- Reduced RAM usage

### 2. Runtime Efficiency
- No runtime checks for unavailable features
- Menu navigation adapts automatically
- Settings persistence only stores relevant data

### 3. Developer Experience
- Clear feature availability at compile time
- Consistent API regardless of board capabilities
- Easy board variant switching

### 4. User Experience
- Menus only show available options
- No confusing disabled features
- Settings persist correctly across board types

## Testing

Use the `feature_flag_test_app.cpp` to verify feature flag behavior:

```bash
# Test full featured board
pio run -e esp32-c6-lcd

# Test WiFi-only board
pio run -e esp32-s3-custom

# Test basic board (no wireless)
pio run -e esp32-s3-round-basic
```

The test app will:
- Display available features at startup
- Show live feature status in menu
- Demonstrate conditional menu behavior
- Validate settings persistence

## Best Practices

### 1. Always Use Feature Guards
```cpp
// Good: Conditional compilation
#if WISP_HAS_WIFI
    setupWiFi();
#endif

// Bad: Runtime checks
if (WISP_HAS_WIFI) {  // This still compiles WiFi code!
    setupWiFi();
}
```

### 2. Provide Stub Methods
```cpp
#if WISP_HAS_BLUETOOTH
  void connectBluetooth() { /* real implementation */ }
#else
  void connectBluetooth() { /* no-op or error message */ }
#endif
```

### 3. Document Feature Requirements
```cpp
/**
 * @brief Initialize network connectivity
 * @requires WISP_HAS_WIFI=1
 * @note This function is a no-op if WiFi is not available
 */
void initNetworking();
```

### 4. Test All Configurations
Always test your application with:
- Full featured boards (all flags enabled)
- Minimal boards (all wireless flags disabled)
- Custom configurations (selective features)

## Migration Guide

### From Fixed Features
If you have existing code that assumes WiFi/Bluetooth are always available:

1. **Add feature guards around wireless code**:
   ```cpp
   // Before
   WiFi.begin(ssid, password);
   
   // After
   #if WISP_HAS_WIFI
   WiFi.begin(ssid, password);
   #endif
   ```

2. **Update menu systems**:
   ```cpp
   // Before: Fixed menu items
   const char* options[] = {"Theme", "WiFi", "Bluetooth"};
   
   // After: Conditional menu items
   static const char* options[8];
   // ... populate based on feature flags
   ```

3. **Add stub methods for compatibility**:
   ```cpp
   #if !WISP_HAS_WIFI
   class WiFi {
   public:
       static void begin(const char*, const char*) { /* no-op */ }
       static bool isConnected() { return false; }
   };
   #endif
   ```

The feature flag system ensures your Wisp Engine applications work optimally across all ESP32 board variants while maintaining clean, maintainable code.
