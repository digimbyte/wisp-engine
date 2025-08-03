// main_with_frame_rate_example.cpp
// Example showing how to integrate the new frame rate management system

// frame_rate_example.cpp - ESP-IDF Native Frame Rate Demo
// ESP-IDF NATIVE - NO ARDUINO

#include "src/system/esp32_common.h"  // ESP-IDF native headers
#include "src/core/game_loop_manager.h"
#include "src/system/app_manager.h"
#include "src/examples/example_app_with_frame_rate.h"

// Mock system components (replace with real implementations)
class MockGraphicsEngine : public GraphicsEngine {
public:
    void init() override { Serial.println("Graphics: Initialized"); }
    void beginFrame() override {}
    void endFrame() override {}
    void clear() override {}
    void drawEntity(const GameEntity& entity) override {}
    void drawRegion(const PhysicsRegion& region) override {}
};

class MockPhysicsEngine : public PhysicsEngine {
public:
    void init() override { Serial.println("Physics: Initialized"); }
    void step(float deltaTime) override {}
    bool checkCollision(const GameEntity& a, const GameEntity& b) override { return false; }
    CollisionResult resolveCollision(GameEntity& a, GameEntity& b) override { return {false, 0, 0}; }
    void updateEntity(GameEntity& entity, float deltaTime) override {}
};

class MockAudioEngine : public AudioEngine {
public:
    void init() override { Serial.println("Audio: Initialized"); }
    void playSound(uint16_t soundId) override {}
    void stopSound(uint16_t soundId) override {}
    void setVolume(float volume) override {}
};

class MockInputController : public InputController {
public:
    void init() override { Serial.println("Input: Initialized"); }
    void update() override {}
    bool isPressed(uint8_t button) override { return false; }
    bool wasPressed(uint8_t button) override { return false; }
    bool wasReleased(uint8_t button) override { return false; }
};

// Global system instances
MockGraphicsEngine graphics;
MockPhysicsEngine physics;
MockAudioEngine audio;
MockInputController input;
GameLoopManager gameLoopManager;
AppManager appManager;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== WispEngine Frame Rate Management Demo ===");
    
    // Initialize system components
    graphics.init();
    physics.init();
    audio.init();
    input.init();
    
    // Get app configuration
    AppHeader appHeader = ExampleFrameRateApp::getAppHeader();
    
    // Validate app header
    if (!AppHeaderUtils::validateHeader(appHeader)) {
        Serial.println("ERROR: Invalid app header!");
        return;
    }
    
    Serial.println("App Header Validation: PASSED");
    Serial.print("App Name: "); Serial.println(appHeader.name);
    Serial.print("Target FPS: "); Serial.println(static_cast<uint8_t>(appHeader.targetFrameRate));
    Serial.print("Minimum FPS: "); Serial.println(static_cast<uint8_t>(appHeader.minimumFrameRate));
    Serial.print("Adaptive Scaling: "); Serial.println(appHeader.allowFrameRateScaling ? "Yes" : "No");
    
    // Check system compatibility
    if (!AppHeaderUtils::checkSystemCompatibility(appHeader)) {
        Serial.println("WARNING: System may not meet app requirements");
    }
    
    // Initialize game loop manager with app-specific frame rate settings
    if (!gameLoopManager.initWithApp(&graphics, &physics, &audio, &input, appHeader)) {
        Serial.println("ERROR: Failed to initialize Game Loop Manager");
        return;
    }
    
    // Initialize app manager
    appManager.init(&gameLoopManager);
    gameLoopManager.setAppManager(&appManager);
    
    // Initialize the example app
    frameRateApp.init(&gameLoopManager);
    
    // Start the game loop
    if (!gameLoopManager.start()) {
        Serial.println("ERROR: Failed to start game loop");
        return;
    }
    
    Serial.println("=== System Initialization Complete ===");
    Serial.println("Press 't' to test frame rate scaling");
    Serial.println("Press 'a' to toggle adaptive scaling");
    Serial.println("Press 'r' to print performance report");
    Serial.println("Press 's' to print detailed stats");
}

void loop() {
    // Handle serial commands for testing
    if (Serial.available()) {
        char command = Serial.read();
        switch (command) {
            case 't':
                frameRateApp.testFrameRateScaling();
                break;
            case 'a':
                frameRateApp.toggleAdaptiveScaling();
                break;
            case 'r':
                frameRateApp.printAppPerformanceReport();
                break;
            case 's':
                gameLoopManager.printPerformanceReport();
                break;
        }
    }
    
    // Update app logic
    frameRateApp.update();
    
    // Main game loop update - handles frame rate limiting internally
    gameLoopManager.update();
}

/* Expected Output:
=== WispEngine Frame Rate Management Demo ===
Graphics: Initialized
Physics: Initialized
Audio: Initialized
Input: Initialized
App Header Validation: PASSED
App Name: FrameRateDemo
Target FPS: 30
Minimum FPS: 15
Adaptive Scaling: Yes
Frame Rate Manager: Target 30 FPS, Min 15 FPS, Adaptive: On
Game Loop Manager initialized with Frame Rate Manager
Game Loop Manager initialized with app-specific frame rate settings
Game Loop started
=== Frame Rate Demo App Started ===
This app demonstrates:
- Target 30 FPS with minimum 15 FPS
- Adaptive frame rate scaling enabled
- Performance monitoring every 5 seconds
- Simple moving entity for load testing
=== System Initialization Complete ===
Press 't' to test frame rate scaling
Press 'a' to toggle adaptive scaling
Press 'r' to print performance report
Press 's' to print detailed stats

=== App Performance Report ===
Current FPS: 30
Target FPS: 30
Frame Drop %: 0.00
Free Heap: 298234 bytes
Player Position: (150, 100)
============================
*/
