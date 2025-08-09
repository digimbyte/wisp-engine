// engine_update_integration.h - Integration point for efficient system updates
#ifndef ENGINE_UPDATE_INTEGRATION_H
#define ENGINE_UPDATE_INTEGRATION_H

#include "bluetooth_manager.h"

namespace WispEngine {
namespace System {

// Call this from the main engine update loop - ONLY processes when needed
inline void updateNetworkingSystems() {
    // BluetoothManager has efficient built-in timing - only updates every 1000ms
    // and skips processing when not enabled/initialized
    BluetoothManager::getInstance().update();
    
    // Future: Add other networking system updates here
    // - WiFi status monitoring (with similar efficient timing)
    // - Network connectivity checks
    // - Hotspot management
    // Each should have built-in timing to avoid excessive processing
}

// Call this during engine initialization
inline void initNetworkingSystems() {
    // BluetoothManager will be initialized on-demand when setEnabled(true) is called
    // This just ensures the singleton is created
    BluetoothManager::getInstance();
}

// Call this during engine shutdown
inline void shutdownNetworkingSystems() {
    BluetoothManager::getInstance().shutdown();
}

} // namespace System
} // namespace WispEngine

#endif // ENGINE_UPDATE_INTEGRATION_H
