// examples/network_test_app.cpp - Network Connectivity Test
// 
// SECURITY MODEL: WiFi configuration is managed ONLY by the engine core UI.
// Apps can only:
// - Query connection status (read-only)
// - Make HTTP requests when connected
// - Test network connectivity
// 
// Apps CANNOT:
// - Configure WiFi credentials
// - Access raw WiFi settings
// - Force WiFi connection/disconnection
// - View sensitive network information
//
// This ensures WiFi security is centralized and not exposed to individual apps.

#include "../src/engine/app/interface.h"

// Required API structures (part of curated API)
struct WispNetworkStatus {
    bool connected = false;
    std::string ssid = "";        // Public SSID name only
    int signalStrength = 0;       // Signal strength in dBm
    std::string ipAddress = "";   // Local IP address
    std::string macAddress = "";  // Device MAC address
};

struct WispNetworkTestResult {
    bool success = false;
    uint32_t latencyMs = 0;
    std::string errorMessage = "";
};

struct WispHTTPRequest {
    std::string method;
    std::string url;
    std::map<std::string, std::string> headers;
    std::string body;
};

class NetworkTestApp : public WispAppBase {
private:
    // Network test modes
    enum NetworkTestMode {
        TEST_WIFI_STATUS,   // WiFi connection status
        TEST_HTTP_GET,      // GET requests
        TEST_HTTP_POST,     // POST requests
        TEST_HTTP_PATCH,    // PATCH requests
        TEST_COUNT
    };
    
    NetworkTestMode currentMode = TEST_WIFI_STATUS;
    
    // WiFi status
    struct WiFiStatus {
        bool connected = false;
        std::string ssid = "";
        int signalStrength = 0; // dBm
        std::string ipAddress = "";
        std::string macAddress = "";
        uint32_t lastUpdate = 0;
    } wifiStatus;
    
    // HTTP test endpoints
    struct HTTPEndpoint {
        std::string name;
        std::string url;
        std::string description;
    };
    
    std::vector<HTTPEndpoint> getEndpoints = {
        {"JSONPlaceholder", "https://jsonplaceholder.typicode.com/posts/1", "Simple GET test"},
        {"HTTPBin", "https://httpbin.org/get", "GET with headers"},
        {"Weather API", "https://api.openweathermap.org/data/2.5/weather?q=London", "API example"},
        {"Status Check", "https://httpstat.us/200", "Status code test"}
    };
    
    std::vector<HTTPEndpoint> postEndpoints = {
        {"JSONPlaceholder", "https://jsonplaceholder.typicode.com/posts", "Create post"},
        {"HTTPBin", "https://httpbin.org/post", "POST test"},
        {"WebHook Test", "https://webhook.site/test", "Webhook test"}
    };
    
    std::vector<HTTPEndpoint> patchEndpoints = {
        {"JSONPlaceholder", "https://jsonplaceholder.typicode.com/posts/1", "Update post"},
        {"HTTPBin", "https://httpbin.org/patch", "PATCH test"}
    };
    
    // Request state
    uint8_t currentEndpointIndex = 0;
    bool requestInProgress = false;
    uint32_t requestStartTime = 0;
    std::string lastResponse = "";
    std::string lastError = "";
    int lastStatusCode = 0;
    
    // Auto-test state
    bool autoTest = false;
    uint32_t lastAutoTest = 0;
    uint32_t autoTestInterval = 5000; // 5 seconds
    
    // Connection monitoring (read-only)
    uint32_t lastWiFiCheck = 0;
    uint32_t wifiCheckInterval = 2000; // 2 seconds

public:
    bool init() override {
        setAppInfo("Network Test", "1.0.0", "Wisp Engine Team");
        
        // Initialize WiFi
        initializeWiFi();
        
        api->print("Network Test App initialized");
        api->print("Controls: Up/Down - Mode, A - Execute, B - Next Endpoint");
        api->print("Start - Auto Test, Select - WiFi Info");
        return true;
    }
    
    void initializeWiFi() {
        // Check current WiFi status only - no configuration access
        api->print("Checking WiFi status...");
        
        // Query connection status from engine (read-only)
        WispNetworkStatus status = api->getNetworkStatus();
        wifiStatus.connected = status.connected;
        
        if (wifiStatus.connected) {
            // Only display public/safe network information
            wifiStatus.ssid = status.ssid;  // Engine provides this safely
            wifiStatus.signalStrength = status.signalStrength;
            wifiStatus.ipAddress = status.ipAddress;
            wifiStatus.macAddress = status.macAddress;
            api->print("WiFi connection detected");
        } else {
            api->print("No WiFi connection available");
            api->print("Configure WiFi through engine settings");
        }
    }
    
    std::string getModeName(NetworkTestMode mode) {
        const char* names[] = {"WiFi Status", "HTTP GET", "HTTP POST", "HTTP PATCH"};
        return names[mode];
    }
    
    std::string formatSignalStrength(int rssi) {
        if (rssi > -50) return "Excellent";
        if (rssi > -60) return "Good";
        if (rssi > -70) return "Fair";
        if (rssi > -80) return "Weak";
        return "Poor";
    }
    
    void update() override {
        uint32_t currentTime = api->getTime();
        
        // Update WiFi status periodically
        if (currentTime - lastWiFiCheck > wifiCheckInterval) {
            updateWiFiStatus();
            lastWiFiCheck = currentTime;
        }
        
        // Handle input
        const WispInputState& input = api->getInput();
        static WispInputState lastInput;
        
        // Mode selection
        if (input.up && !lastInput.up) {
            currentMode = (NetworkTestMode)((currentMode + 1) % TEST_COUNT);
            currentEndpointIndex = 0;
            api->print("Network Mode: " + getModeName(currentMode));
        }
        if (input.down && !lastInput.down) {
            currentMode = (NetworkTestMode)((currentMode - 1 + TEST_COUNT) % TEST_COUNT);
            currentEndpointIndex = 0;
            api->print("Network Mode: " + getModeName(currentMode));
        }
        
        // Execute current test
        if (input.buttonA && !lastInput.buttonA && !requestInProgress) {
            executeCurrentTest();
        }
        
        // Next endpoint
        if (input.buttonB && !lastInput.buttonB) {
            nextEndpoint();
        }
        
        // Auto test toggle
        if (input.start && !lastInput.start) {
            autoTest = !autoTest;
            api->print("Auto Test: " + std::string(autoTest ? "ON" : "OFF"));
        }
        
        // WiFi reconnect (removed - not app responsibility)
        if (input.select && !lastInput.select) {
            api->print("WiFi management is handled by engine settings");
            api->print("Use device settings to configure WiFi");
        }
        
        lastInput = input;
        
        // Auto test execution
        if (autoTest && !requestInProgress && currentTime - lastAutoTest > autoTestInterval) {
            if (wifiStatus.connected) {
                executeCurrentTest();
                lastAutoTest = currentTime;
            }
        }
        
        // Check request timeout
        if (requestInProgress && currentTime - requestStartTime > 10000) {
            requestInProgress = false;
            lastError = "Request timeout";
            api->print("Request timed out");
        }
    }
    
    void updateWiFiStatus() {
        // Query WiFi status from engine (read-only)
        WispNetworkStatus status = api->getNetworkStatus();
        bool wasConnected = wifiStatus.connected;
        
        wifiStatus.connected = status.connected;
        wifiStatus.signalStrength = status.signalStrength;
        wifiStatus.lastUpdate = api->getTime();
        
        if (wifiStatus.connected && !wasConnected) {
            api->print("WiFi connection established");
        } else if (!wifiStatus.connected && wasConnected) {
            api->print("WiFi connection lost");
        }
    }
    
    void executeCurrentTest() {
        if (!wifiStatus.connected) {
            lastError = "WiFi not connected";
            api->print("Error: WiFi not connected");
            return;
        }
        
        requestInProgress = true;
        requestStartTime = api->getTime();
        lastResponse = "";
        lastError = "";
        lastStatusCode = 0;
        
        switch (currentMode) {
            case TEST_WIFI_STATUS:
                executeWiFiStatusTest();
                break;
            case TEST_HTTP_GET:
                executeGETTest();
                break;
            case TEST_HTTP_POST:
                executePOSTTest();
                break;
            case TEST_HTTP_PATCH:
                executePATCHTest();
                break;
        }
    }
    
    void executeWiFiStatusTest() {
        // Network connectivity test using engine API (no direct WiFi access)
        api->print("Testing network connectivity...");
        
        // Use engine's network test capability
        WispNetworkTestResult result = api->testNetworkConnectivity();
        
        if (result.success) {
            lastResponse = "Connectivity OK - " + std::to_string(result.latencyMs) + "ms";
            lastStatusCode = 200;
            api->print("Network test: PASSED");
        } else {
            lastError = "Connectivity failed: " + result.errorMessage;
            api->print("Network test: FAILED");
        }
        
        requestInProgress = false;
    }
    
    void executeGETTest() {
        if (currentEndpointIndex >= getEndpoints.size()) {
            currentEndpointIndex = 0;
        }
        
        const HTTPEndpoint& endpoint = getEndpoints[currentEndpointIndex];
        api->print("GET: " + endpoint.name);
        
        // Simulate HTTP GET request
        WispHTTPRequest request;
        request.method = "GET";
        request.url = endpoint.url;
        request.headers["User-Agent"] = "WispEngine/1.0";
        request.headers["Accept"] = "application/json";
        
        bool success = simulateHTTPRequest(request);
        requestInProgress = false;
        
        if (success) {
            api->print("GET request successful");
        } else {
            api->print("GET request failed");
        }
    }
    
    void executePOSTTest() {
        if (currentEndpointIndex >= postEndpoints.size()) {
            currentEndpointIndex = 0;
        }
        
        const HTTPEndpoint& endpoint = postEndpoints[currentEndpointIndex];
        api->print("POST: " + endpoint.name);
        
        // Simulate HTTP POST request
        WispHTTPRequest request;
        request.method = "POST";
        request.url = endpoint.url;
        request.headers["Content-Type"] = "application/json";
        request.headers["User-Agent"] = "WispEngine/1.0";
        request.body = R"({"title": "Test Post", "body": "Test content", "userId": 1})";
        
        bool success = simulateHTTPRequest(request);
        requestInProgress = false;
        
        if (success) {
            api->print("POST request successful");
        } else {
            api->print("POST request failed");
        }
    }
    
    void executePATCHTest() {
        if (currentEndpointIndex >= patchEndpoints.size()) {
            currentEndpointIndex = 0;
        }
        
        const HTTPEndpoint& endpoint = patchEndpoints[currentEndpointIndex];
        api->print("PATCH: " + endpoint.name);
        
        // Simulate HTTP PATCH request
        WispHTTPRequest request;
        request.method = "PATCH";
        request.url = endpoint.url;
        request.headers["Content-Type"] = "application/json";
        request.headers["User-Agent"] = "WispEngine/1.0";
        request.body = R"({"title": "Updated Title"})";
        
        bool success = simulateHTTPRequest(request);
        requestInProgress = false;
        
        if (success) {
            api->print("PATCH request successful");
        } else {
            api->print("PATCH request failed");
        }
    }
    
    bool simulateHTTPRequest(const WispHTTPRequest& request) {
        // Simulate HTTP request with random success/failure
        // In real implementation, would use ESP32 HTTP client
        
        bool success = api->randomInt(0, 100) > 15; // 85% success rate
        
        if (success) {
            lastStatusCode = 200;
            
            if (request.method == "GET") {
                lastResponse = R"({"id": 1, "title": "Sample Data", "status": "success"})";
            } else if (request.method == "POST") {
                lastResponse = R"({"id": 101, "created": true, "status": "success"})";
            } else if (request.method == "PATCH") {
                lastResponse = R"({"id": 1, "updated": true, "status": "success"})";
            }
        } else {
            lastStatusCode = api->randomInt(0, 1) ? 404 : 500;
            lastError = lastStatusCode == 404 ? "Not Found" : "Internal Server Error";
        }
        
        return success;
    }
    
    void nextEndpoint() {
        switch (currentMode) {
            case TEST_HTTP_GET:
                currentEndpointIndex = (currentEndpointIndex + 1) % getEndpoints.size();
                api->print("GET Endpoint: " + getEndpoints[currentEndpointIndex].name);
                break;
            case TEST_HTTP_POST:
                currentEndpointIndex = (currentEndpointIndex + 1) % postEndpoints.size();
                api->print("POST Endpoint: " + postEndpoints[currentEndpointIndex].name);
                break;
            case TEST_HTTP_PATCH:
                currentEndpointIndex = (currentEndpointIndex + 1) % patchEndpoints.size();
                api->print("PATCH Endpoint: " + patchEndpoints[currentEndpointIndex].name);
                break;
        }
    }
    
    void reconnectWiFi() {
        // Removed - WiFi management is engine responsibility
        api->print("WiFi configuration managed by engine settings");
        api->print("Apps can only query connection status");
    }
    
    void render() override {
        // Clear with dark background
        api->drawRect(0, 0, 320, 240, WispColor(10, 20, 30), 0);
        
        // Draw title
        api->drawText("NETWORK TEST", 160, 10, WispColor(255, 255, 255), 10);
        
        // Draw current mode
        api->drawText(getModeName(currentMode), 160, 25, WispColor(200, 200, 255), 9);
        
        // Draw WiFi status (always visible)
        renderWiFiStatus();
        
        // Draw mode-specific content
        switch (currentMode) {
            case TEST_WIFI_STATUS:
                renderWiFiDetails();
                break;
            case TEST_HTTP_GET:
                renderHTTPTest("GET", getEndpoints, currentEndpointIndex);
                break;
            case TEST_HTTP_POST:
                renderHTTPTest("POST", postEndpoints, currentEndpointIndex);
                break;
            case TEST_HTTP_PATCH:
                renderHTTPTest("PATCH", patchEndpoints, currentEndpointIndex);
                break;
        }
        
        // Draw request status
        renderRequestStatus();
        
        // Draw controls
        api->drawText("Up/Down: Mode  A: Execute  B: Next", 10, 210, WispColor(180, 180, 180), 8);
        api->drawText("Start: Auto Test  Select: Info", 10, 225, WispColor(180, 180, 180), 8);
    }
    
    void renderWiFiStatus() {
        int y = 45;
        
        // Connection status
        std::string statusText = "WiFi: ";
        WispColor statusColor;
        if (wifiStatus.connected) {
            statusText += "CONNECTED";
            statusColor = WispColor(0, 255, 0);
        } else {
            statusText += "DISCONNECTED";
            statusColor = WispColor(255, 0, 0);
        }
        
        api->drawText(statusText, 10, y, statusColor, 8);
        
        if (wifiStatus.connected) {
            api->drawText("SSID: " + wifiStatus.ssid, 150, y, WispColor(200, 200, 200), 8);
            
            std::string signalText = "Signal: " + std::to_string(wifiStatus.signalStrength) + "dBm (" + 
                                   formatSignalStrength(wifiStatus.signalStrength) + ")";
            api->drawText(signalText, 10, y + 15, WispColor(200, 200, 200), 8);
            
            api->drawText("IP: " + wifiStatus.ipAddress, 10, y + 30, WispColor(200, 200, 200), 8);
        }
    }
    
    void renderWiFiDetails() {
        int y = 95;
        
        if (wifiStatus.connected) {
            api->drawText("Network Information:", 10, y, WispColor(255, 255, 255), 8);
            api->drawText("MAC Address: " + wifiStatus.macAddress, 10, y + 15, WispColor(200, 200, 200), 8);
            
            uint32_t uptime = (api->getTime() - wifiStatus.lastUpdate) / 1000;
            api->drawText("Connection Uptime: " + std::to_string(uptime) + "s", 10, y + 30, WispColor(200, 200, 200), 8);
            
            api->drawText("Press A to ping test", 10, y + 50, WispColor(255, 255, 0), 8);
        } else {
            api->drawText("WiFi Disconnected", 10, y, WispColor(255, 100, 100), 8);
            api->drawText("Configure WiFi in engine settings", 10, y + 15, WispColor(255, 255, 0), 8);
        }
    }
    
    void renderHTTPTest(const std::string& method, const std::vector<HTTPEndpoint>& endpoints, uint8_t index) {
        int y = 95;
        
        if (index < endpoints.size()) {
            const HTTPEndpoint& endpoint = endpoints[index];
            
            api->drawText("Endpoint: " + endpoint.name, 10, y, WispColor(255, 255, 255), 8);
            api->drawText(endpoint.description, 10, y + 15, WispColor(200, 200, 200), 8);
            
            // Truncate URL if too long
            std::string url = endpoint.url;
            if (url.length() > 45) {
                url = url.substr(0, 42) + "...";
            }
            api->drawText("URL: " + url, 10, y + 30, WispColor(150, 150, 255), 8);
            
            std::string countText = "(" + std::to_string(index + 1) + "/" + std::to_string(endpoints.size()) + ")";
            api->drawText(countText, 250, y, WispColor(180, 180, 180), 8);
        }
        
        // Auto test indicator
        if (autoTest) {
            api->drawText("AUTO TEST: ON", 200, y + 45, WispColor(0, 255, 0), 8);
        }
    }
    
    void renderRequestStatus() {
        int y = 155;
        
        if (requestInProgress) {
            api->drawText("Request in progress...", 10, y, WispColor(255, 255, 0), 8);
            
            uint32_t elapsed = (api->getTime() - requestStartTime) / 1000;
            api->drawText("Elapsed: " + std::to_string(elapsed) + "s", 10, y + 15, WispColor(200, 200, 200), 8);
        } else {
            if (lastStatusCode > 0) {
                std::string statusText = "Status: " + std::to_string(lastStatusCode);
                WispColor statusColor = (lastStatusCode == 200) ? WispColor(0, 255, 0) : WispColor(255, 100, 100);
                api->drawText(statusText, 10, y, statusColor, 8);
            }
            
            if (!lastResponse.empty()) {
                std::string response = lastResponse;
                if (response.length() > 50) {
                    response = response.substr(0, 47) + "...";
                }
                api->drawText("Response: " + response, 10, y + 15, WispColor(200, 255, 200), 8);
            }
            
            if (!lastError.empty()) {
                api->drawText("Error: " + lastError, 10, y + 30, WispColor(255, 100, 100), 8);
            }
        }
    }
    
    void cleanup() override {
        // No WiFi disconnection - engine manages this
        api->print("Network Test App cleaned up");
    }
};

// Export function for the engine
extern "C" WispAppBase* createNetworkTestApp() {
    return new NetworkTestApp();
}

extern "C" void destroyNetworkTestApp(WispAppBase* app) {
    delete app;
}
