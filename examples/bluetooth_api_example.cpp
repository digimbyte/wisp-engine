// bluetooth_api_example.cpp
// Example demonstrating how ROM/apps use the Bluetooth API with engine error handling

#include "../src/engine/app/curated_api.h"

// Example ROM/App that demonstrates Bluetooth usage with proper error handling
class BluetoothExampleApp {
private:
    WispCuratedAPI* api;
    bool bluetoothInitialized;
    std::string deviceName;
    
public:
    BluetoothExampleApp(WispCuratedAPI* curatedAPI) : api(curatedAPI), bluetoothInitialized(false), deviceName("WispGameDevice") {}
    
    bool initialize() {
        api->print("=== Bluetooth Example App Starting ===");
        
        // First, check if Bluetooth is supported on this board
        if (!api->isBluetoothSupported()) {
            api->printWarning("Bluetooth not supported on this board - running in offline mode");
            return true; // Continue without Bluetooth
        }
        
        api->print("Bluetooth supported: " + api->getBluetoothStatus());
        
        // Try to enable Bluetooth
        if (api->enableBluetooth(deviceName)) {
            bluetoothInitialized = true;
            api->print("Bluetooth initialized successfully");
            
            // Try to start appropriate Bluetooth service based on board capabilities
            if (!startBluetoothServices()) {
                api->printWarning("Failed to start Bluetooth services, but continuing");
            }
            
            return true;
        } else {
            api->printError("Failed to initialize Bluetooth - check permissions");
            return false;
        }
    }
    
    bool startBluetoothServices() {
        // Try BLE first (more common, lower power)
        if (api->startBLEAdvertising(deviceName, "12345678-1234-1234-1234-123456789ABC")) {
            api->print("BLE advertising started");
            return true;
        }
        
        // Fall back to Bluetooth Classic if available
        if (api->startBTEServer(deviceName)) {
            api->print("Bluetooth Classic server started");
            return true;
        }
        
        api->printError("Could not start any Bluetooth services");
        return false;
    }
    
    void update() {
        if (!bluetoothInitialized) {
            return; // Skip Bluetooth operations if not initialized
        }
        
        // Check for incoming data
        std::string receivedData = api->receiveBluetoothData();
        if (!receivedData.empty()) {
            api->print("Received Bluetooth data: " + receivedData);
            processBluetoothCommand(receivedData);
        }
        
        // Send periodic status updates
        static uint32_t lastStatusSend = 0;
        uint32_t currentTime = api->getTime();
        
        if (currentTime - lastStatusSend > 5000) { // Every 5 seconds
            sendStatusUpdate();
            lastStatusSend = currentTime;
        }
    }
    
    void processBluetoothCommand(const std::string& command) {
        if (command == "status") {
            sendStatusUpdate();
        } else if (command == "ping") {
            if (!api->sendBluetoothData("pong")) {
                api->printWarning("Failed to send pong response");
            }
        } else if (command.substr(0, 4) == "say:") {
            std::string message = command.substr(4);
            api->print("Remote says: " + message);
            
            // Echo back the message
            if (!api->sendBluetoothData("echo:" + message)) {
                api->printWarning("Failed to echo message");
            }
        } else {
            api->print("Unknown Bluetooth command: " + command);
        }
    }
    
    void sendStatusUpdate() {
        if (!bluetoothInitialized) {
            return;
        }
        
        std::string status = "status:device=" + deviceName + 
                           ",uptime=" + std::to_string(api->getTime()) + 
                           ",connected=" + (api->isBluetoothConnected() ? "yes" : "no");
        
        if (!api->sendBluetoothData(status)) {
            api->printWarning("Failed to send status update");
        }
    }
    
    void shutdown() {
        if (bluetoothInitialized) {
            api->print("Shutting down Bluetooth services...");
            api->stopBLEAdvertising();
            api->stopBTEServer();
            api->disableBluetooth();
            bluetoothInitialized = false;
        }
        
        api->print("=== Bluetooth Example App Shutdown Complete ===");
    }
    
    // Demonstrate error handling scenarios
    void testErrorHandling() {
        api->print("=== Testing Bluetooth Error Handling ===");
        
        // Test 1: Try to use BLE on a board that doesn't support it
        api->print("Test 1: Attempting BLE operations...");
        if (!api->sendBLEData("test")) {
            // This will fail on ESP32-C6 if it only supports BLE, or on boards with no Bluetooth
            api->print("BLE operation failed as expected (board-specific limitation)");
        }
        
        // Test 2: Try to use Bluetooth Classic on a board that doesn't support it  
        api->print("Test 2: Attempting Bluetooth Classic operations...");
        if (!api->sendBTEData("test")) {
            // This will fail on ESP32-C6 (BLE-only) but work on ESP32-S3 (dual-mode)
            api->print("Bluetooth Classic operation failed as expected (board-specific limitation)");
        }
        
        // Test 3: Try to send data when not connected
        api->print("Test 3: Attempting to send data without connection...");
        if (!api->sendBluetoothData("test")) {
            api->print("Send failed as expected (no connection)");
        }
        
        // Test 4: Try to send oversized BLE data
        api->print("Test 4: Attempting to send oversized BLE data...");
        std::string largeData(300, 'x'); // 300 characters, exceeds BLE MTU
        if (!api->sendBLEData(largeData)) {
            api->print("Large BLE data send failed as expected (size limit exceeded)");
        }
        
        api->print("=== Error Handling Tests Complete ===");
    }
};

// Example usage in a ROM/App main loop
void wispAppMain() {
    // Get the curated API instance (provided by engine)
    WispCuratedAPI* api = &WISP_API;
    
    // Create the Bluetooth example app
    BluetoothExampleApp app(api);
    
    // Initialize the app
    if (!app.initialize()) {
        api->printError("App initialization failed");
        return;
    }
    
    // Run error handling tests
    app.testErrorHandling();
    
    // Main application loop
    api->print("Starting main application loop...");
    
    for (int frame = 0; frame < 100; frame++) { // Run for 100 frames as example
        // Begin frame processing
        if (!api->beginFrame()) {
            api->printError("Frame processing disabled (emergency mode?)");
            break;
        }
        
        // Update app logic
        api->beginUpdate();
        app.update();
        api->endUpdate();
        
        // Render (if needed)
        api->beginRender();
        // ... rendering code would go here ...
        api->endRender();
        
        // End frame
        api->endFrame();
        
        // Simple delay for demonstration
        delay(100); // 100ms = ~10 FPS for demo
    }
    
    // Shutdown
    app.shutdown();
    api->print("Application completed successfully");
}

// Expected output examples based on board type:

/* 
ESP32-C6 (BLE-only) expected output:
=== Bluetooth Example App Starting ===
Bluetooth supported: Bluetooth (BLE): ready
Bluetooth initialized successfully
BLE advertising started
=== Testing Bluetooth Error Handling ===
Test 1: Attempting BLE operations...
[BLE operations work normally]
Test 2: Attempting Bluetooth Classic operations...
WISP API ERROR: Bluetooth Classic not supported on this board (board supports: BLE)
Bluetooth Classic operation failed as expected (board-specific limitation)
Test 3: Attempting to send data without connection...
Send failed as expected (no connection)
Test 4: Attempting to send oversized BLE data...
WISP API ERROR: BLE data too large (max 244 bytes)
Large BLE data send failed as expected (size limit exceeded)
=== Error Handling Tests Complete ===
*/

/*
ESP32-S3 (Dual-mode) expected output:
=== Bluetooth Example App Starting ===
Bluetooth supported: Bluetooth (BLE+BTE): ready
Bluetooth initialized successfully
BLE advertising started
=== Testing Bluetooth Error Handling ===
Test 1: Attempting BLE operations...
[BLE operations work normally]
Test 2: Attempting Bluetooth Classic operations...
[BTE operations work normally]
Test 3: Attempting to send data without connection...
Send failed as expected (no connection)
Test 4: Attempting to send oversized BLE data...
WISP API ERROR: BLE data too large (max 244 bytes)
Large BLE data send failed as expected (size limit exceeded)
=== Error Handling Tests Complete ===
*/

/*
Board with no Bluetooth expected output:
=== Bluetooth Example App Starting ===
Bluetooth not supported on this board - running in offline mode
=== Testing Bluetooth Error Handling ===
Test 1: Attempting BLE operations...
WISP API ERROR: BLE not supported on this board (board supports: NULL)
BLE operation failed as expected (board-specific limitation)
Test 2: Attempting Bluetooth Classic operations...
WISP API ERROR: Bluetooth Classic not supported on this board (board supports: NULL)
Bluetooth Classic operation failed as expected (board-specific limitation)
Test 3: Attempting to send data without connection...
WISP API ERROR: No Bluetooth support on this board
Send failed as expected (no connection)
=== Error Handling Tests Complete ===
*/
