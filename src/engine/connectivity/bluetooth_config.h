// bluetooth_config.h
#pragma once

// Wisp Engine - Bluetooth Configuration Helper
// Provides unified Bluetooth type detection and utility macros
// 
// This header automatically detects the board's Bluetooth capabilities and provides
// convenient macros for conditional compilation and feature detection.
//
// Bluetooth Types:
// - BLE (Bluetooth Low Energy): Power-efficient, suitable for sensors, beacons, simple data
// - BTE (Bluetooth Classic/Traditional): Higher power, suitable for audio, file transfer
// - NULL: No Bluetooth support available

#include "wisp_config.h"

// === BLUETOOTH TYPE DETECTION ===
// These macros come from user-defined board configuration
// Default to NULL if not defined by user

#ifndef BLUETOOTH_TYPE_BLE
    #define BLUETOOTH_TYPE_BLE          0
#endif
#ifndef BLUETOOTH_TYPE_BTE
    #define BLUETOOTH_TYPE_BTE          0
#endif
#ifndef BLUETOOTH_TYPE_NULL
    #define BLUETOOTH_TYPE_NULL         1
#endif

// === BLUETOOTH TYPE VALIDATION ===
// Ensure exactly one Bluetooth type is selected
#if (BLUETOOTH_TYPE_BLE + BLUETOOTH_TYPE_BTE + BLUETOOTH_TYPE_NULL) != 1
    #error "Exactly one Bluetooth type must be enabled: BLE, BTE, or NULL"
#endif

// === BLUETOOTH CAPABILITY DETECTION MACROS ===
#define WISP_BLUETOOTH_IS_BLE_ONLY      (BLUETOOTH_TYPE_BLE && !BLUETOOTH_TYPE_BTE)
#define WISP_BLUETOOTH_IS_BTE_ONLY      (BLUETOOTH_TYPE_BTE && !BLUETOOTH_TYPE_BLE)
#define WISP_BLUETOOTH_IS_DUAL_MODE     (BLUETOOTH_TYPE_BLE && BLUETOOTH_TYPE_BTE)
#define WISP_BLUETOOTH_IS_DISABLED      (BLUETOOTH_TYPE_NULL)

// Convenience macros for feature availability
#define WISP_HAS_BLE                    BLUETOOTH_TYPE_BLE
#define WISP_HAS_BTE                    BLUETOOTH_TYPE_BTE
#define WISP_HAS_ANY_BLUETOOTH          (BLUETOOTH_TYPE_BLE || BLUETOOTH_TYPE_BTE)

// === BLUETOOTH FEATURE STRINGS ===
// Human-readable strings for debugging and logging
#if WISP_BLUETOOTH_IS_BLE_ONLY
    #define WISP_BLUETOOTH_TYPE_STRING      "BLE"
    #define WISP_BLUETOOTH_DESCRIPTION      "Bluetooth 5.0 Low Energy"
#elif WISP_BLUETOOTH_IS_BTE_ONLY
    #define WISP_BLUETOOTH_TYPE_STRING      "BTE"
    #define WISP_BLUETOOTH_DESCRIPTION      "Bluetooth Classic"
#elif WISP_BLUETOOTH_IS_DUAL_MODE
    #define WISP_BLUETOOTH_TYPE_STRING      "BLE+BTE"
    #define WISP_BLUETOOTH_DESCRIPTION      "Bluetooth Classic + Low Energy"
#else
    #define WISP_BLUETOOTH_TYPE_STRING      "NULL"
    #define WISP_BLUETOOTH_DESCRIPTION      "No Bluetooth"
#endif

// === BLUETOOTH PROFILE SUPPORT ===
// Define which Bluetooth profiles are supported based on type

// BLE Profiles (GATT-based)
#if WISP_HAS_BLE
    #define WISP_SUPPORTS_BLE_GATT          1
    #define WISP_SUPPORTS_BLE_GAMEPAD       1      // HID over GATT for game controllers
    #define WISP_SUPPORTS_BLE_AUDIO         1      // Audio over BLE (LE Audio)
    #define WISP_SUPPORTS_BLE_MESH          1      // Bluetooth Mesh networking
    #define WISP_SUPPORTS_BLE_BEACON        1      // iBeacon/Eddystone advertising
    #define WISP_SUPPORTS_BLE_UART          1      // Nordic UART Service
#else
    #define WISP_SUPPORTS_BLE_GATT          0
    #define WISP_SUPPORTS_BLE_GAMEPAD       0
    #define WISP_SUPPORTS_BLE_AUDIO         0
    #define WISP_SUPPORTS_BLE_MESH          0
    #define WISP_SUPPORTS_BLE_BEACON        0
    #define WISP_SUPPORTS_BLE_UART          0
#endif

// BTE Profiles (Classic Bluetooth)
#if WISP_HAS_BTE
    #define WISP_SUPPORTS_BTE_SPP           1      // Serial Port Profile
    #define WISP_SUPPORTS_BTE_A2DP          1      // Advanced Audio Distribution
    #define WISP_SUPPORTS_BTE_HID           1      // Human Interface Device
    #define WISP_SUPPORTS_BTE_OBEX          1      // Object Exchange (file transfer)
    #define WISP_SUPPORTS_BTE_HFP           1      // Hands-Free Profile
    #define WISP_SUPPORTS_BTE_AVRCP         1      // Audio/Video Remote Control
#else
    #define WISP_SUPPORTS_BTE_SPP           0
    #define WISP_SUPPORTS_BTE_A2DP          0
    #define WISP_SUPPORTS_BTE_HID           0
    #define WISP_SUPPORTS_BTE_OBEX          0
    #define WISP_SUPPORTS_BTE_HFP           0
    #define WISP_SUPPORTS_BTE_AVRCP         0
#endif

// === BLUETOOTH SECURITY LEVELS ===
// Define supported security and authentication levels
#if WISP_HAS_ANY_BLUETOOTH
    #define WISP_BLUETOOTH_SECURITY_NONE    1      // No security (open)
    #define WISP_BLUETOOTH_SECURITY_AUTH    1      // Authentication required
    #define WISP_BLUETOOTH_SECURITY_ENCRYPT 1      // Encryption required
    #define WISP_BLUETOOTH_SECURITY_BOND    1      // Bonding/pairing required
#else
    #define WISP_BLUETOOTH_SECURITY_NONE    0
    #define WISP_BLUETOOTH_SECURITY_AUTH    0
    #define WISP_BLUETOOTH_SECURITY_ENCRYPT 0
    #define WISP_BLUETOOTH_SECURITY_BOND    0
#endif

// === BLUETOOTH POWER MANAGEMENT ===
// Power consumption characteristics based on type
#if WISP_BLUETOOTH_IS_BLE_ONLY
    #define WISP_BLUETOOTH_POWER_CONSUMPTION_UA     50     // ~50 µA average (BLE)
    #define WISP_BLUETOOTH_CAN_DEEP_SLEEP          1      // Can maintain connections in deep sleep
    #define WISP_BLUETOOTH_WAKE_ON_CONNECT         1      // Can wake from sleep on connection
#elif WISP_BLUETOOTH_IS_BTE_ONLY
    #define WISP_BLUETOOTH_POWER_CONSUMPTION_UA     15000  // ~15 mA average (Classic)
    #define WISP_BLUETOOTH_CAN_DEEP_SLEEP          0      // Cannot deep sleep while connected
    #define WISP_BLUETOOTH_WAKE_ON_CONNECT         0      // Cannot wake from deep sleep
#elif WISP_BLUETOOTH_IS_DUAL_MODE
    #define WISP_BLUETOOTH_POWER_CONSUMPTION_UA     8000   // ~8 mA average (mixed mode)
    #define WISP_BLUETOOTH_CAN_DEEP_SLEEP          1      // BLE connections support deep sleep
    #define WISP_BLUETOOTH_WAKE_ON_CONNECT         1      // BLE can wake on connection
#else
    #define WISP_BLUETOOTH_POWER_CONSUMPTION_UA     0      // No Bluetooth
    #define WISP_BLUETOOTH_CAN_DEEP_SLEEP          1      // No power consumption
    #define WISP_BLUETOOTH_WAKE_ON_CONNECT         0      // No Bluetooth to wake on
#endif

// === CONDITIONAL COMPILATION HELPERS ===
// Use these macros to conditionally compile Bluetooth-related code

// Include BLE-specific code
#if WISP_HAS_BLE
    #define WISP_BLE_CODE(code)             code
    #define WISP_BLE_ONLY_CODE(code)        do { if (WISP_BLUETOOTH_IS_BLE_ONLY) { code } } while(0)
#else
    #define WISP_BLE_CODE(code)             // BLE not available
    #define WISP_BLE_ONLY_CODE(code)        // BLE not available
#endif

// Include BTE-specific code
#if WISP_HAS_BTE
    #define WISP_BTE_CODE(code)             code
    #define WISP_BTE_ONLY_CODE(code)        do { if (WISP_BLUETOOTH_IS_BTE_ONLY) { code } } while(0)
#else
    #define WISP_BTE_CODE(code)             // BTE not available
    #define WISP_BTE_ONLY_CODE(code)        // BTE not available
#endif

// Include code only if any Bluetooth is available
#if WISP_HAS_ANY_BLUETOOTH
    #define WISP_BLUETOOTH_CODE(code)       code
#else
    #define WISP_BLUETOOTH_CODE(code)       // No Bluetooth available
#endif

// === RUNTIME BLUETOOTH TYPE DETECTION ===
#ifdef __cplusplus
namespace WispEngine {
    namespace Bluetooth {
        
        // Enum for runtime type detection
        enum class BluetoothType {
            None = 0,
            BLE = 1,
            BTE = 2,
            Dual = 3
        };
        
        // Runtime type detection function
        constexpr BluetoothType getBluetoothType() {
            #if WISP_BLUETOOTH_IS_DUAL_MODE
                return BluetoothType::Dual;
            #elif WISP_BLUETOOTH_IS_BLE_ONLY
                return BluetoothType::BLE;
            #elif WISP_BLUETOOTH_IS_BTE_ONLY
                return BluetoothType::BTE;
            #else
                return BluetoothType::None;
            #endif
        }
        
        // Capability check functions
        constexpr bool supportsBLE() { return WISP_HAS_BLE; }
        constexpr bool supportsBTE() { return WISP_HAS_BTE; }
        constexpr bool supportsAnyBluetooth() { return WISP_HAS_ANY_BLUETOOTH; }
        
        // Power consumption estimate (in microamps)
        constexpr uint32_t getPowerConsumption() { 
            return WISP_BLUETOOTH_POWER_CONSUMPTION_UA; 
        }
        
        // Feature availability
        constexpr bool canDeepSleep() { return WISP_BLUETOOTH_CAN_DEEP_SLEEP; }
        constexpr bool canWakeOnConnect() { return WISP_BLUETOOTH_WAKE_ON_CONNECT; }
    }
}
#endif // __cplusplus

// === DEBUGGING AND LOGGING HELPERS ===
// Macros for conditional debug output based on Bluetooth availability

#ifdef WISP_DEBUG
    #if WISP_HAS_ANY_BLUETOOTH
        #define WISP_BT_DEBUG(fmt, ...) \
            WISP_DEBUG("[BT:%s] " fmt, WISP_BLUETOOTH_TYPE_STRING, ##__VA_ARGS__)
    #else
        #define WISP_BT_DEBUG(fmt, ...) // No Bluetooth debug when disabled
    #endif
    
    #if WISP_HAS_BLE
        #define WISP_BLE_DEBUG(fmt, ...) \
            WISP_DEBUG("[BLE] " fmt, ##__VA_ARGS__)
    #else
        #define WISP_BLE_DEBUG(fmt, ...) // No BLE debug when disabled
    #endif
    
    #if WISP_HAS_BTE
        #define WISP_BTE_DEBUG(fmt, ...) \
            WISP_DEBUG("[BTE] " fmt, ##__VA_ARGS__)
    #else
        #define WISP_BTE_DEBUG(fmt, ...) // No BTE debug when disabled
    #endif
#else
    #define WISP_BT_DEBUG(fmt, ...)      // Debug disabled
    #define WISP_BLE_DEBUG(fmt, ...)     // Debug disabled
    #define WISP_BTE_DEBUG(fmt, ...)     // Debug disabled
#endif

