# Wisp Engine - Bluetooth API System

## Overview

The Wisp Engine implements a comprehensive Bluetooth API system that handles board-specific capabilities and provides proper error responses to ROM/app code. The system is designed for the engine side to validate hardware capabilities and return health check information to applications.

## Architecture

### Engine-Side Responsibilities

1. **Board Capability Detection**: Automatically detect BTE vs BLE support based on target board
2. **API Validation**: Validate all Bluetooth API calls against board capabilities  
3. **Error Handling**: Return proper error codes when unsupported features are accessed
4. **Health Checks**: Provide connection status and diagnostic information
5. **Permission Enforcement**: Check network access permissions before Bluetooth operations

### App/ROM-Side Interface

- Apps access Bluetooth **only** through the curated API
- No direct hardware access allowed
- All operations return success/failure status
- Detailed error messages logged by engine
- Graceful degradation when Bluetooth unavailable

## Board Support Matrix

| Board | Bluetooth Type | BLE Support | BTE Support | Power Consumption |
|-------|----------------|-------------|-------------|-------------------|
| ESP32-C6 | BLE-only | ✅ Full | ❌ None | ~50 µA |
| ESP32-S3 | Dual-mode | ✅ Full | ✅ Full | ~8 mA |
| Generic | NULL | ❌ None | ❌ None | 0 µA |

## API Functions

### Capability Detection

```cpp
bool isBluetoothSupported();        // Check if any Bluetooth available
bool isBluetoothEnabled();          // Check if Bluetooth stack active
std::string getBluetoothStatus();   // Get detailed status report
```

### Generic Bluetooth (Auto-detect)

```cpp
bool enableBluetooth(const std::string& deviceName = "WispEngine");
void disableBluetooth();
bool sendBluetoothData(const std::string& data);
std::string receiveBluetoothData();
bool isBluetoothConnected();
```

### BLE-Specific Functions (ESP32-C6/S3)

```cpp
bool startBLEAdvertising(const std::string& deviceName, const std::string& serviceUUID = "");
void stopBLEAdvertising();
bool sendBLEData(const std::string& data);
std::string receiveBLEData();
bool isBLEConnected();
```

### Bluetooth Classic Functions (ESP32-S3 only)

```cpp
bool startBTEServer(const std::string& deviceName);
void stopBTEServer();
bool sendBTEData(const std::string& data);
std::string receiveBTEData();
bool isBTEConnected();
```

## Error Handling System

### Error Types and Responses

1. **Unsupported Feature Errors**
   ```
   "BLE not supported on this board (board supports: BTE)"
   "Bluetooth Classic not supported on this board (board supports: BLE)"
   "No Bluetooth support on this board"
   ```

2. **Permission Errors**
   ```
   "App does not have network access permission for Bluetooth"
   "App does not have network access permission for BLE"
   ```

3. **Connection Errors**
   ```
   "Bluetooth Classic not connected"
   "Failed to initialize Bluetooth"
   ```

4. **Data Validation Errors**
   ```
   "Cannot send empty BLE data"
   "BLE data too large (max 244 bytes)"
   ```

### Error Response Flow

1. App calls Bluetooth API function
2. Engine validates board capabilities using compile-time macros
3. If unsupported: Log error + return false/empty
4. If supported: Check permissions and connection status
5. Forward to appropriate hardware implementation
6. Return success/failure status to app

## Example Usage Patterns

### Basic Bluetooth Communication

```cpp
// App checks board capabilities
if (!api->isBluetoothSupported()) {
    // Graceful fallback to offline mode
    return;
}

// Enable Bluetooth with error handling
if (!api->enableBluetooth("MyGameDevice")) {
    api->printError("Bluetooth init failed");
    return;
}

// Send data with automatic type detection
if (!api->sendBluetoothData("Hello World")) {
    api->printWarning("Send failed - no connection?");
}
```

### Board-Specific Feature Usage

```cpp
// Try BLE first (more efficient)
if (api->startBLEAdvertising("GameDevice", "12345678-1234-1234-1234-123456789ABC")) {
    api->print("Using BLE for communication");
} 
// Fall back to Classic Bluetooth
else if (api->startBTEServer("GameDevice")) {
    api->print("Using Bluetooth Classic for communication");
}
else {
    api->print("No Bluetooth services available");
}
```

### Error Handling Best Practices

```cpp
// Check capabilities before attempting operations
void attemptBluetoothOperation() {
    if (!api->isBluetoothSupported()) {
        // Silent fallback - no error logging needed
        return;
    }
    
    // Attempt operation with error checking
    if (!api->sendBLEData("test")) {
        // Engine logs specific error, app handles gracefully
        api->printWarning("BLE operation failed - using alternative method");
        tryAlternativeMethod();
    }
}
```

## Implementation Details

### Compile-Time Board Detection

```cpp
// Engine automatically includes appropriate board config
#include "../connectivity/bluetooth_config.h"

bool WispCuratedAPI::isBluetoothSupported() {
    return WISP_HAS_ANY_BLUETOOTH;  // Resolves at compile time
}

bool WispCuratedAPI::sendBLEData(const std::string& data) {
    if (!WISP_HAS_BLE) {
        recordError("BLE not supported on this board (board supports: " + 
                   String(WISP_BLUETOOTH_TYPE_STRING) + ")");
        return false;
    }
    // ... continue with BLE implementation
}
```

### Runtime Validation

```cpp
bool WispCuratedAPI::enableBluetooth(const std::string& deviceName) {
    // Permission check
    if (!appPermissions.canAccessNetwork) {
        recordError("App does not have network access permission for Bluetooth");
        return false;
    }
    
    // Capability check
    if (!isBluetoothSupported()) {
        recordError("Bluetooth not supported on this board");
        return false;
    }
    
    // Forward to hardware implementation
    return BluetoothManager::begin(String(deviceName.c_str()));
}
```

### Health Check Integration

```cpp
std::string WispCuratedAPI::getBluetoothStatus() {
    if (!WISP_HAS_ANY_BLUETOOTH) {
        return "Bluetooth not supported on this board";
    }
    
    String status = "Bluetooth (" + String(WISP_BLUETOOTH_TYPE_STRING) + "): ";
    
    if (isBluetoothEnabled()) {
        status += BluetoothManager::getStatusReport();
    } else {
        status += "disabled";
    }
    
    return std::string(status.c_str());
}
```

## Integration with Engine Systems

### Resource Management

- Bluetooth operations are quota-limited like other engine resources
- Connection state tracked for proper cleanup during app shutdown
- Memory usage monitored and reported through health checks

### Security and Permissions

- All Bluetooth access requires `canAccessNetwork` permission
- Apps cannot directly access hardware Bluetooth APIs
- Engine validates all parameters and sanitizes inputs

### Error Reporting and Logging

- Detailed error messages logged by engine for debugging
- Apps receive boolean success/failure status
- Emergency mode triggered on excessive API errors

## Future Enhancements

1. **Advanced BLE Features**
   - GATT server/client implementation
   - Custom service/characteristic definitions
   - Bluetooth Mesh networking support

2. **Enhanced Security**
   - Bluetooth pairing and bonding
   - Encrypted communication channels
   - Device authentication mechanisms

3. **Performance Optimizations**
   - Connection pooling for multiple devices
   - Automatic reconnection handling
   - Power management integration

4. **Extended Compatibility**
   - Support for additional ESP32 variants
   - Generic Bluetooth adapter support
   - Cross-platform abstraction layer

## Summary

The Wisp Engine Bluetooth API system provides:

- **Automatic board capability detection** at compile time
- **Robust error handling** with meaningful error messages
- **Permission-based access control** for security
- **Health check integration** for diagnostics
- **Graceful degradation** when Bluetooth unavailable
- **Clean separation** between engine and app responsibilities

This ensures that ROM/app code can safely attempt Bluetooth operations on any board, with the engine providing appropriate error responses and health status information based on the actual hardware capabilities.
