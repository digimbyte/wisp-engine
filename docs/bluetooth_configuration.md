# Bluetooth Configuration System

The Wisp Engine includes a comprehensive Bluetooth configuration system that automatically detects and configures Bluetooth capabilities based on the target board's hardware specifications.

## Bluetooth Types

The system supports three Bluetooth configuration types:

- **BLE (Bluetooth Low Energy)**: Power-efficient, suitable for sensors, beacons, simple data transfer
- **BTE (Bluetooth Classic/Traditional)**: Higher power, suitable for audio streaming, file transfer, legacy devices
- **NULL**: No Bluetooth support available

## Board Support

### ESP32-C6
- **Type**: BLE-only
- **Profiles**: GATT, HID over GATT, LE Audio, Mesh, Beacon, UART Service
- **Power**: ~50 µA average consumption
- **Deep Sleep**: Supported with connection maintenance
- **Wake on Connect**: Yes

### ESP32-S3
- **Type**: Dual-mode (BLE + BTE)
- **BLE Profiles**: GATT, HID over GATT, LE Audio, Mesh, Beacon, UART Service
- **BTE Profiles**: A2DP, HID, SPP, OBEX, HFP, AVRCP
- **Power**: ~8 mA average consumption (mixed mode)
- **Deep Sleep**: Supported for BLE connections
- **Wake on Connect**: Yes (BLE only)

## Usage

### Include the Configuration Header

```cpp
#include "src/engine/connectivity/bluetooth_config.h"
```

### Compile-time Detection

```cpp
#if WISP_HAS_BLE
    // BLE-specific code
    setupBLEGATTServer();
#endif

#if WISP_HAS_BTE
    // Bluetooth Classic specific code
    setupA2DPProfile();
#endif

#if WISP_HAS_ANY_BLUETOOTH
    // Code for any Bluetooth type
    initializeBluetoothStack();
#endif
```

### Runtime Detection

```cpp
using namespace WispEngine::Bluetooth;

if (supportsBLE()) {
    std::cout << "BLE is available" << std::endl;
}

if (supportsBTE()) {
    std::cout << "Bluetooth Classic is available" << std::endl;
}

BluetoothType type = getBluetoothType();
switch (type) {
    case BluetoothType::BLE:
        std::cout << "BLE-only platform" << std::endl;
        break;
    case BluetoothType::BTE:
        std::cout << "Bluetooth Classic only platform" << std::endl;
        break;
    case BluetoothType::Dual:
        std::cout << "Dual-mode Bluetooth platform" << std::endl;
        break;
    case BluetoothType::None:
        std::cout << "No Bluetooth support" << std::endl;
        break;
}
```

### Conditional Compilation Helpers

The system provides convenient macros for conditional compilation:

```cpp
// Include code only if BLE is available
WISP_BLE_CODE({
    advertiseBLEService("Wisp Game Engine");
});

// Include code only if Bluetooth Classic is available
WISP_BTE_CODE({
    startA2DPAudioStreaming();
});

// Include code for any Bluetooth type
WISP_BLUETOOTH_CODE({
    printBluetoothStatus();
});
```

### Debug Logging

Bluetooth-specific debug macros are available:

```cpp
WISP_BT_DEBUG("Bluetooth initialized: %s", WISP_BLUETOOTH_TYPE_STRING);
WISP_BLE_DEBUG("BLE GATT server started on %s", deviceName);
WISP_BTE_DEBUG("A2DP profile registered");
```

## Power Management

The configuration system provides power consumption estimates and capabilities:

```cpp
uint32_t powerConsumption = WispEngine::Bluetooth::getPowerConsumption();
std::cout << "Bluetooth power consumption: " << powerConsumption << " µA" << std::endl;

if (WispEngine::Bluetooth::canDeepSleep()) {
    std::cout << "Can maintain connections in deep sleep" << std::endl;
}

if (WispEngine::Bluetooth::canWakeOnConnect()) {
    std::cout << "Can wake on incoming connections" << std::endl;
}
```

## Profile Support Detection

Check which Bluetooth profiles are supported:

```cpp
// BLE Profiles
#if WISP_SUPPORTS_BLE_GATT
    initBLEGATT();
#endif

#if WISP_SUPPORTS_BLE_GAMEPAD
    registerGamepadService();
#endif

#if WISP_SUPPORTS_BLE_AUDIO
    setupLEAudio();
#endif

// BTE Profiles
#if WISP_SUPPORTS_BTE_A2DP
    initA2DPSink();
#endif

#if WISP_SUPPORTS_BTE_HID
    registerHIDService();
#endif

#if WISP_SUPPORTS_BTE_SPP
    openSerialPortProfile();
#endif
```

## Configuration Macros

### Board-specific Configuration (set in board config headers):
- `BLUETOOTH_TYPE_BLE` - Enable BLE support (0 or 1)
- `BLUETOOTH_TYPE_BTE` - Enable Bluetooth Classic support (0 or 1)
- `BLUETOOTH_TYPE_NULL` - No Bluetooth support (0 or 1)

### Generated Detection Macros:
- `WISP_HAS_BLE` - BLE is available
- `WISP_HAS_BTE` - Bluetooth Classic is available
- `WISP_HAS_ANY_BLUETOOTH` - Any Bluetooth type is available
- `WISP_BLUETOOTH_IS_BLE_ONLY` - Only BLE is available
- `WISP_BLUETOOTH_IS_BTE_ONLY` - Only Bluetooth Classic is available
- `WISP_BLUETOOTH_IS_DUAL_MODE` - Both BLE and Classic are available
- `WISP_BLUETOOTH_IS_DISABLED` - No Bluetooth support

### Profile Support Macros:
- `WISP_SUPPORTS_BLE_*` - BLE profile availability
- `WISP_SUPPORTS_BTE_*` - Bluetooth Classic profile availability

### Power Management Macros:
- `WISP_BLUETOOTH_POWER_CONSUMPTION_UA` - Power consumption in microamps
- `WISP_BLUETOOTH_CAN_DEEP_SLEEP` - Can maintain connections in deep sleep
- `WISP_BLUETOOTH_WAKE_ON_CONNECT` - Can wake on incoming connections

## Build Configuration

When building for specific platforms, the appropriate platform macro should be defined:

```bash
# For ESP32-C6 (BLE-only)
platformio run -e esp32-c6-lcd -D PLATFORM_C6

# For ESP32-S3 (Dual-mode)
platformio run -e esp32-s3-round -D PLATFORM_S3
```

To disable Bluetooth on ESP32-S3 modules without wireless capabilities:

```ini
; platformio.ini
[env:esp32-s3-basic]
build_flags = 
    -DPLATFORM_S3
    -DBLUETOOTH_TYPE_BLE=0
    -DBLUETOOTH_TYPE_BTE=0  
    -DBLUETOOTH_TYPE_NULL=1
```

## Adding New Board Support

To add support for a new board:

1. Create a board configuration header in `boards/` directory
2. Define the appropriate `BLUETOOTH_TYPE_*` macros
3. Add platform detection to `bluetooth_config.h`
4. Update this documentation

Example for a new board with BTE-only support:

```cpp
// boards/new_board_config.h
#define BLUETOOTH_TYPE_BLE          0
#define BLUETOOTH_TYPE_BTE          1  
#define BLUETOOTH_TYPE_NULL         0
```

## Testing

A test file is provided at `tests/bluetooth_config_test.cpp` to validate the configuration system. Compile with the appropriate platform macro to test each board configuration.

## Summary

This Bluetooth configuration system provides:

- **Automatic detection** of board Bluetooth capabilities
- **Compile-time optimization** by excluding unused code
- **Runtime detection** for dynamic behavior
- **Power management** information for battery optimization
- **Profile support** detection for feature availability
- **Debug logging** helpers for development
- **Easy extensibility** for new boards and Bluetooth types

The system ensures that your code compiles efficiently for each target platform while providing a unified API for Bluetooth functionality across different hardware configurations.
