/*
 * Feature Flag Test App
 * 
 * Demonstrates conditional compilation of WiFi and Bluetooth features
 * based on board capabilities defined in platformio.ini build flags.
 * 
 * Build configurations:
 * - esp32-c6-lcd: Full features (WiFi + Bluetooth)
 * - esp32-s3-round-wireless: WiFi + Bluetooth enabled
 * - esp32-s3-round-basic: All wireless features disabled
 * - esp32-s3-custom: WiFi only, no Bluetooth
 * 
 * The menu system automatically adapts based on available features.
 */

#include "../src/system/wisp_curated_api.h"
#include "../src/system/settings.h"
#include "../src/system/ui/panels/menu.h"

class FeatureFlagTestApp : public WispApp {
private:
    WispCuratedAPI api;
    Settings engineSettings;
    bool initialized;
    uint32_t lastFeatureCheck;

public:
    FeatureFlagTestApp() : api(this), initialized(false), lastFeatureCheck(0) {}

    bool init() override {
        api.print("=== WISP ENGINE FEATURE FLAG TEST ===");
        api.print("Board: " + getBoardInfo());
        api.print("Checking available features...");
        
        showFeatureStatus();
        
        // Initialize Settings with NVS
        if (!engineSettings.init()) {
            api.print("ERROR: Failed to initialize engine settings");
            return false;
        }
        api.print("✓ Engine settings initialized");

        // Initialize menu system with conditional features
        if (!init(&api, &engineSettings)) {
            api.print("ERROR: Failed to initialize menu system");
            return false;
        }
        api.print("✓ Menu system initialized with available features");

        // Activate the main menu
        activate();
        
        initialized = true;
        lastFeatureCheck = millis();
        
        api.print("Feature flag test ready!");
        return true;
    }

    void update(const WispInputState& input) override {
        if (!initialized) return;

        // Update current menu panel
        if (currentPanel) {
            currentPanel->update(input);
        }
        
        // Periodically show feature status
        uint32_t now = millis();
        if (now - lastFeatureCheck > 10000) { // Every 10 seconds
            showFeatureStatus();
            lastFeatureCheck = now;
        }
    }

    void render() override {
        if (!initialized) {
            renderLoadingScreen();
            return;
        }

        // Render current menu panel
        if (currentPanel) {
            currentPanel->render();
        }
        
        // Show feature indicators
        renderFeatureIndicators();
    }

    void cleanup() override {
        if (initialized) {
            cleanup();
            api.print("Feature flag test cleaned up");
        }
    }

private:
    String getBoardInfo() {
        String boardInfo = "Unknown";
        
#ifdef PLATFORM_C6
        boardInfo = "ESP32-C6";
  #ifdef BOARD_ESP32_C6_LCD_1_47
        boardInfo += " 1.47\" LCD";
  #endif
#elif defined(PLATFORM_S3)
        boardInfo = "ESP32-S3";
  #ifdef BOARD_ESP32_S3_ROUND_1_28
        boardInfo += " 1.28\" Round";
  #endif
  #ifdef BOARD_ESP32_S3_CUSTOM
        boardInfo += " Custom";
  #endif
#endif

        return boardInfo;
    }

    void showFeatureStatus() {
        api.print("\n=== FEATURE STATUS ===");
        
        // WiFi feature
#if WISP_HAS_WIFI
        api.print("WiFi: AVAILABLE");
        bool wifiEnabled = engineSettings.getWiFiEnabled();
        api.print("  - Current state: " + String(wifiEnabled ? "ENABLED" : "DISABLED"));
  #if WISP_HAS_WIFI_DIRECT
        api.print("  - WiFi Direct: AVAILABLE");
  #else
        api.print("  - WiFi Direct: NOT AVAILABLE");
  #endif
#else
        api.print("WiFi: NOT AVAILABLE (disabled by build flags)");
#endif

        // Bluetooth feature
#if WISP_HAS_BLUETOOTH
        api.print("Bluetooth: AVAILABLE");
        bool btEnabled = engineSettings.getBluetoothEnabled();
        api.print("  - Current state: " + String(btEnabled ? "ENABLED" : "DISABLED"));
        String btName = engineSettings.getBluetoothDeviceName();
        api.print("  - Device name: " + btName);
  #if WISP_HAS_BLUETOOTH_CLASSIC
        api.print("  - Classic Bluetooth: AVAILABLE");
  #else
        api.print("  - Classic Bluetooth: NOT AVAILABLE");
  #endif
#else
        api.print("Bluetooth: NOT AVAILABLE (disabled by build flags)");
#endif

        // External storage
#if WISP_HAS_EXTERNAL_STORAGE
        api.print("External Storage: AVAILABLE");
#else
        api.print("External Storage: NOT AVAILABLE");
#endif

        // Theme (always available)
        api.print("Theme Settings: AVAILABLE");
        uint16_t primaryColor = engineSettings.getThemePrimaryColor();
        uint16_t accentColor = engineSettings.getThemeAccentColor();
        api.print("  - Primary color: 0x" + String(primaryColor, HEX));
        api.print("  - Accent color: 0x" + String(accentColor, HEX));
        
        api.print("======================\n");
    }

    void renderLoadingScreen() {
        api.drawRect(0, 0, 320, 240, WispColor(10, 15, 25), 10);
        api.drawText("FEATURE FLAG TEST", 160, 80, WispColor(100, 150, 255), 3);
        api.drawText("Checking board capabilities...", 160, 120, WispColor(150, 150, 150), 1);
        
        // Loading animation
        int dots = (millis() / 500) % 4;
        for (int i = 0; i < dots; i++) {
            api.drawText(".", 200 + i * 10, 140, WispColor(100, 100, 100), 1);
        }
    }

    void renderFeatureIndicators() {
        // Feature status bar at bottom
        api.drawRect(0, 220, 320, 20, WispColor(0, 0, 0, 150), 0);
        
        int x = 5;
        
        // WiFi indicator
#if WISP_HAS_WIFI
        bool wifiEnabled = engineSettings.getWiFiEnabled();
        WispColor wifiColor = wifiEnabled ? WispColor(0, 255, 0) : WispColor(255, 100, 100);
        api.drawText("WiFi", x, 225, wifiColor, 1);
        x += 35;
#else
        api.drawText("WiFi:N/A", x, 225, WispColor(100, 100, 100), 1);
        x += 55;
#endif

        // Bluetooth indicator
#if WISP_HAS_BLUETOOTH
        bool btEnabled = engineSettings.getBluetoothEnabled();
        WispColor btColor = btEnabled ? WispColor(0, 100, 255) : WispColor(100, 100, 100);
        api.drawText("BT", x, 225, btColor, 1);
        x += 25;
#else
        api.drawText("BT:N/A", x, 225, WispColor(100, 100, 100), 1);
        x += 40;
#endif

        // External storage indicator
#if WISP_HAS_EXTERNAL_STORAGE
        api.drawText("SD", x, 225, WispColor(255, 255, 0), 1);
        x += 25;
#else
        api.drawText("SD:N/A", x, 225, WispColor(100, 100, 100), 1);
        x += 40;
#endif

        // Board type
        String board;
#ifdef PLATFORM_C6
        board = "C6";
#elif defined(PLATFORM_S3)
        board = "S3";
#else
        board = "??";
#endif
        api.drawText(board.c_str(), 280, 225, WispColor(200, 200, 200), 1);

        // Feature count
        int featureCount = 0;
#if WISP_HAS_WIFI
        featureCount++;
#endif
#if WISP_HAS_BLUETOOTH
        featureCount++;
#endif
#if WISP_HAS_EXTERNAL_STORAGE
        featureCount++;
#endif
        
        String features = String(featureCount) + "/3";
        api.drawText(features.c_str(), 250, 225, WispColor(150, 150, 150), 1);
    }
};

// Test functions to validate conditional compilation
void printCompileTimeFeatures() {
    Serial.println("=== COMPILE-TIME FEATURE FLAGS ===");
    
#if WISP_HAS_WIFI
    Serial.println("WISP_HAS_WIFI: 1 (ENABLED)");
#else
    Serial.println("WISP_HAS_WIFI: 0 (DISABLED)");
#endif

#if WISP_HAS_BLUETOOTH
    Serial.println("WISP_HAS_BLUETOOTH: 1 (ENABLED)");
#else
    Serial.println("WISP_HAS_BLUETOOTH: 0 (DISABLED)");
#endif

#if WISP_HAS_BLUETOOTH_CLASSIC
    Serial.println("WISP_HAS_BLUETOOTH_CLASSIC: 1 (ENABLED)");
#else
    Serial.println("WISP_HAS_BLUETOOTH_CLASSIC: 0 (DISABLED)");
#endif

#if WISP_HAS_WIFI_DIRECT
    Serial.println("WISP_HAS_WIFI_DIRECT: 1 (ENABLED)");
#else
    Serial.println("WISP_HAS_WIFI_DIRECT: 0 (DISABLED)");
#endif

#if WISP_HAS_EXTERNAL_STORAGE
    Serial.println("WISP_HAS_EXTERNAL_STORAGE: 1 (ENABLED)");
#else
    Serial.println("WISP_HAS_EXTERNAL_STORAGE: 0 (DISABLED)");
#endif

    Serial.println("================================");
}

// Global app instance
FeatureFlagTestApp app;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    printCompileTimeFeatures();
    app.init();
}

void loop() {
    WispInputState input = getInput();
    app.update(input);
    app.render();
    delay(16); // ~60 FPS
}
