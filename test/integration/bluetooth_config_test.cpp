// bluetooth_config_test.cpp
// Test file to validate Bluetooth configuration detection

#include <iostream>
#include <string>

// Test with ESP32-C6 configuration (BLE-only)
#define PLATFORM_C6
#include "../src/engine/connectivity/bluetooth_config.h"

void testC6Configuration() {
    std::cout << "=== ESP32-C6 Bluetooth Configuration Test ===" << std::endl;
    std::cout << "Bluetooth Type: " << WISP_BLUETOOTH_TYPE_STRING << std::endl;
    std::cout << "Description: " << WISP_BLUETOOTH_DESCRIPTION << std::endl;
    std::cout << "Has BLE: " << (WISP_HAS_BLE ? "YES" : "NO") << std::endl;
    std::cout << "Has BTE: " << (WISP_HAS_BTE ? "YES" : "NO") << std::endl;
    std::cout << "Is BLE Only: " << (WISP_BLUETOOTH_IS_BLE_ONLY ? "YES" : "NO") << std::endl;
    std::cout << "Is BTE Only: " << (WISP_BLUETOOTH_IS_BTE_ONLY ? "YES" : "NO") << std::endl;
    std::cout << "Is Dual Mode: " << (WISP_BLUETOOTH_IS_DUAL_MODE ? "YES" : "NO") << std::endl;
    std::cout << "Is Disabled: " << (WISP_BLUETOOTH_IS_DISABLED ? "YES" : "NO") << std::endl;
    std::cout << "Power Consumption: " << WISP_BLUETOOTH_POWER_CONSUMPTION_UA << " µA" << std::endl;
    std::cout << "Can Deep Sleep: " << (WISP_BLUETOOTH_CAN_DEEP_SLEEP ? "YES" : "NO") << std::endl;
    std::cout << "Can Wake on Connect: " << (WISP_BLUETOOTH_WAKE_ON_CONNECT ? "YES" : "NO") << std::endl;
    
    // Test C++ namespace functions
    using namespace WispEngine::Bluetooth;
    std::cout << "Runtime Type: ";
    switch(getBluetoothType()) {
        case BluetoothType::None: std::cout << "None"; break;
        case BluetoothType::BLE: std::cout << "BLE"; break;
        case BluetoothType::BTE: std::cout << "BTE"; break;
        case BluetoothType::Dual: std::cout << "Dual"; break;
    }
    std::cout << std::endl;
    
    // Test profile support
    std::cout << "BLE GATT Support: " << (WISP_SUPPORTS_BLE_GATT ? "YES" : "NO") << std::endl;
    std::cout << "BLE Gamepad Support: " << (WISP_SUPPORTS_BLE_GAMEPAD ? "YES" : "NO") << std::endl;
    std::cout << "BTE A2DP Support: " << (WISP_SUPPORTS_BTE_A2DP ? "YES" : "NO") << std::endl;
    std::cout << "BTE HID Support: " << (WISP_SUPPORTS_BTE_HID ? "YES" : "NO") << std::endl;
    std::cout << std::endl;
}

#undef PLATFORM_C6

// Test with ESP32-S3 configuration (Dual-mode)
#define PLATFORM_S3
#include "../src/engine/connectivity/bluetooth_config.h"

void testS3Configuration() {
    std::cout << "=== ESP32-S3 Bluetooth Configuration Test ===" << std::endl;
    std::cout << "Bluetooth Type: " << WISP_BLUETOOTH_TYPE_STRING << std::endl;
    std::cout << "Description: " << WISP_BLUETOOTH_DESCRIPTION << std::endl;
    std::cout << "Has BLE: " << (WISP_HAS_BLE ? "YES" : "NO") << std::endl;
    std::cout << "Has BTE: " << (WISP_HAS_BTE ? "YES" : "NO") << std::endl;
    std::cout << "Is BLE Only: " << (WISP_BLUETOOTH_IS_BLE_ONLY ? "YES" : "NO") << std::endl;
    std::cout << "Is BTE Only: " << (WISP_BLUETOOTH_IS_BTE_ONLY ? "YES" : "NO") << std::endl;
    std::cout << "Is Dual Mode: " << (WISP_BLUETOOTH_IS_DUAL_MODE ? "YES" : "NO") << std::endl;
    std::cout << "Is Disabled: " << (WISP_BLUETOOTH_IS_DISABLED ? "YES" : "NO") << std::endl;
    std::cout << "Power Consumption: " << WISP_BLUETOOTH_POWER_CONSUMPTION_UA << " µA" << std::endl;
    std::cout << "Can Deep Sleep: " << (WISP_BLUETOOTH_CAN_DEEP_SLEEP ? "YES" : "NO") << std::endl;
    std::cout << "Can Wake on Connect: " << (WISP_BLUETOOTH_WAKE_ON_CONNECT ? "YES" : "NO") << std::endl;
    
    // Test C++ namespace functions
    using namespace WispEngine::Bluetooth;
    std::cout << "Runtime Type: ";
    switch(getBluetoothType()) {
        case BluetoothType::None: std::cout << "None"; break;
        case BluetoothType::BLE: std::cout << "BLE"; break;
        case BluetoothType::BTE: std::cout << "BTE"; break;
        case BluetoothType::Dual: std::cout << "Dual"; break;
    }
    std::cout << std::endl;
    
    // Test profile support
    std::cout << "BLE GATT Support: " << (WISP_SUPPORTS_BLE_GATT ? "YES" : "NO") << std::endl;
    std::cout << "BLE Gamepad Support: " << (WISP_SUPPORTS_BLE_GAMEPAD ? "YES" : "NO") << std::endl;
    std::cout << "BTE A2DP Support: " << (WISP_SUPPORTS_BTE_A2DP ? "YES" : "NO") << std::endl;
    std::cout << "BTE HID Support: " << (WISP_SUPPORTS_BTE_HID ? "YES" : "NO") << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "Wisp Engine - Bluetooth Configuration Test" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    // Note: This test file demonstrates the concept but won't compile
    // as written because we can't redefine the same headers with 
    // different platform macros in a single translation unit.
    //
    // In practice, the platform macro is set during build configuration
    // and each board would be built separately.
    
    std::cout << "This test demonstrates the Bluetooth configuration system." << std::endl;
    std::cout << "For actual testing, compile with:" << std::endl;
    std::cout << "  -DPLATFORM_C6  for ESP32-C6 (BLE-only)" << std::endl;
    std::cout << "  -DPLATFORM_S3  for ESP32-S3 (BLE+BTE)" << std::endl;
    
    return 0;
}

// Example of conditional compilation usage in real code:
void exampleBluetoothCode() {
    WISP_BLUETOOTH_CODE({
        // This code only compiles if any Bluetooth is available
        std::cout << "Initializing Bluetooth..." << std::endl;
    });
    
    WISP_BLE_CODE({
        // This code only compiles if BLE is available
        std::cout << "Setting up BLE GATT server..." << std::endl;
    });
    
    WISP_BTE_CODE({
        // This code only compiles if Classic Bluetooth is available
        std::cout << "Setting up A2DP audio profile..." << std::endl;
    });
    
    // Runtime conditional execution
    if (WispEngine::Bluetooth::supportsBLE()) {
        std::cout << "BLE is supported on this platform" << std::endl;
    }
    
    if (WispEngine::Bluetooth::supportsBTE()) {
        std::cout << "Bluetooth Classic is supported on this platform" << std::endl;
    }
}
