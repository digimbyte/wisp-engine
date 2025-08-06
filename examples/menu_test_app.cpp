/*
 * Menu System Test App
 * 
 * Demonstrates the complete menu system with:
 * - Main menu (Play/Settings navigation)
 * - Settings carousel (Theme/WiFi/Bluetooth/Profile)
 * - NVS-persisted engine settings
 * - Theme customization with color palettes
 * - WiFi/Bluetooth configuration storage
 * 
 * Controls:
 * - D-pad: Navigate menus and change settings
 * - A: Select/Confirm
 * - B: Back/Cancel
 */

#include "../src/system/wisp_curated_api.h"
#include "../src/system/settings.h"
#include "../src/system/ui/panels/menu.h"

class MenuTestApp : public WispApp {
private:
    WispCuratedAPI api;
    Settings engineSettings;
    bool initialized;

public:
    MenuTestApp() : api(this), initialized(false) {}

    bool init() override {
        // Initialize Settings with NVS
        if (!engineSettings.init()) {
            api.print("ERROR: Failed to initialize engine settings");
            return false;
        }

        // Initialize menu system with API and Settings
        if (!init(&api, &engineSettings)) {
            api.print("ERROR: Failed to initialize menu system");
            return false;
        }

        api.print("Menu system initialized successfully");
        api.print("Use D-pad to navigate, A to select, B to go back");
        
        // Activate the main menu
        activate();
        
        initialized = true;
        return true;
    }

    void update(const WispInputState& input) override {
        if (!initialized) return;

        // Update current menu panel
        if (currentPanel) {
            currentPanel->update(input);
        }
    }

    void render() override {
        if (!initialized) {
            // Show loading screen
            api.drawRect(0, 0, 320, 240, WispColor(0, 0, 0), 10);
            api.drawText("Initializing Menu System...", 160, 120, WispColor(255, 255, 255), 2);
            return;
        }

        // Render current menu panel
        if (currentPanel) {
            currentPanel->render();
        }
        
        // Show current settings status in corner
        renderSettingsStatus();
    }

    void cleanup() override {
        if (initialized) {
            // Cleanup menu system
            cleanup();
            api.print("Menu system cleaned up");
        }
    }

private:
    void renderSettingsStatus() {
        // Show current theme colors and connection status in small text at bottom
        uint16_t primaryColor = engineSettings.getThemePrimaryColor();
        uint16_t accentColor = engineSettings.getThemeAccentColor();
        bool wifiEnabled = engineSettings.getWiFiEnabled();
        bool bluetoothEnabled = engineSettings.getBluetoothEnabled();
        
        // Status bar background
        api.drawRect(0, 220, 320, 20, WispColor(0, 0, 0, 128), 0);
        
        // Theme colors preview
        api.drawRect(5, 222, 8, 16, WispColor(primaryColor), 1);
        api.drawRect(15, 222, 8, 16, WispColor(accentColor), 1);
        
        // Connection status
        const char* wifiStatus = wifiEnabled ? "WiFi:ON" : "WiFi:OFF";
        const char* btStatus = bluetoothEnabled ? "BT:ON" : "BT:OFF";
        
        api.drawText(wifiStatus, 30, 225, WispColor(150, 150, 150), 1);
        api.drawText(btStatus, 80, 225, WispColor(150, 150, 150), 1);
        
        // Current menu indicator
        if (currentPanel) {
            api.drawText(currentPanel->getTitle(), 250, 225, WispColor(100, 100, 100), 1);
        }
    }
};

// Global app instance
MenuTestApp app;

void setup() {
    app.init();
}

void loop() {
    WispInputState input = getInput();
    app.update(input);
    app.render();
    delay(16); // ~60 FPS
}
