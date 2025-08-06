# Network Test Application

## Purpose
Tests network connectivity status and HTTP API methods (GET, POST, PATCH) for ESP32 networking capabilities. **Does NOT configure WiFi** - only queries connection status.

## Security Model
This application follows the secure network API model:
- ✅ **Apps CAN**: Query connection status, make HTTP requests, test connectivity
- ❌ **Apps CANNOT**: Configure WiFi, access credentials, force connections

WiFi configuration is handled exclusively through the engine's core UI for security.

## Files
- `network_test_app.cpp` - Main test application

## Controls
- **Up/Down**: Switch network test modes
- **A Button**: Execute current test
- **B Button**: Next endpoint
- **Start**: Toggle auto-test mode
- **Select**: Show WiFi info (read-only)

## Network Test Modes

### 1. Network Status (Read-Only)
- Connection status monitoring
- Signal strength reporting  
- IP address display
- Network information (no configuration)

### 2. HTTP GET
- GET request testing
- Multiple test endpoints
- Response validation
- Header handling

### 3. HTTP POST
- POST request testing
- JSON payload transmission
- Content-Type handling
- Response processing

### 4. HTTP PATCH
- PATCH request testing
- Partial update operations
- REST API validation
- Error handling

## Features Tested
- ✅ Network connection status (read-only)
- ✅ Connection monitoring and reporting
- ✅ HTTP GET requests
- ✅ HTTP POST requests  
- ✅ HTTP PATCH requests
- ✅ Request/response handling
- ✅ Error detection and recovery
- ✅ Performance metrics
- ✅ Auto-test capabilities
- ❌ WiFi configuration (engine-only)

## Test Endpoints
The application includes various test endpoints:
- JSONPlaceholder API for testing
- HTTPBin for request/response validation
- Weather API examples
- Custom webhook testing

## Network Information
Displays comprehensive network status:
- SSID and connection state
- Signal strength (dBm and quality)
- IP address and MAC address
- Connection uptime
- Request timing and status codes

## Usage
This test validates the ESP32's networking capabilities and ensures reliable HTTP communication for games that require online features, leaderboards, or remote configuration.

**Important**: WiFi must be configured through the engine's settings UI before running this test. The app will only display connection status and cannot modify WiFi settings.
