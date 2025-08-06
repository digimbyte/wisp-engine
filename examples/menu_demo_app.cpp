/*
 * Complete Menu System Demo
 * 
 * This demo showcases the full menu system architecture:
 * 
 * MENU STRUCTURE:
 * ├── Main Menu
 * │   ├── Play (demo game)
 * │   └── Settings
 * └── Settings (carousel)
 *     ├── Theme Settings
 *     │   ├── Primary Color (8 color palette)
 *     │   └── Accent Color (8 color palette)
 *     ├── WiFi Settings  
 *     │   ├── Enable/Disable WiFi
 *     │   ├── Network SSID (read-only display)
 *     │   └── Connection Status
 *     ├── Bluetooth Settings
 *     │   ├── Enable/Disable Bluetooth
 *     │   └── Device Name
 *     └── Profile Settings
 *         └── (Future: User profiles, preferences)
 * 
 * PERSISTENCE:
 * - All settings stored in NVS (Non-Volatile Storage)
 * - WiFi credentials encrypted with mbedTLS
 * - Settings survive device reboots
 * - Engine-level isolation from app data
 * 
 * SECURITY:
 * - WiFi settings API is read-only for apps
 * - No credential exposure to application code
 * - Encrypted storage for sensitive data
 * 
 * Controls:
 * - D-pad/Arrow keys: Navigation
 * - A/Enter: Select/Confirm
 * - B/Escape: Back/Cancel
 */

#include "../src/system/wisp_curated_api.h"
#include "../src/system/settings.h"
#include "../src/system/ui/panels/menu.h"

class MenuDemoApp : public WispApp {
private:
    WispCuratedAPI api;
    Settings engineSettings;
    bool initialized;
    bool inGameMode;
    uint32_t lastUpdate;
    
    // Demo game state
    struct DemoGame {
        float playerX, playerY;
        float velocityX, velocityY;
        uint16_t primaryColor, accentColor;
        bool gameActive;
        uint32_t score;
    } game;

public:
    MenuDemoApp() : api(this), initialized(false), inGameMode(false), lastUpdate(0) {
        // Initialize demo game
        game.playerX = 160;
        game.playerY = 120;
        game.velocityX = 2.0f;
        game.velocityY = 1.5f;
        game.primaryColor = 0xFFFF;
        game.accentColor = 0x07E0;
        game.gameActive = false;
        game.score = 0;
    }

    bool init() override {
        api.print("=== WISP ENGINE MENU SYSTEM DEMO ===");
        api.print("Initializing NVS storage...");
        
        // Initialize Settings with NVS
        if (!engineSettings.init()) {
            api.print("ERROR: Failed to initialize engine settings");
            api.print("Check NVS partition and flash configuration");
            return false;
        }
        api.print("✓ Engine settings initialized");

        // Initialize menu system with API and Settings
        api.print("Initializing menu panels...");
        if (!init(&api, &engineSettings)) {
            api.print("ERROR: Failed to initialize menu system");
            return false;
        }
        api.print("✓ Menu system initialized");

        // Load current theme for demo game
        game.primaryColor = engineSettings.getThemePrimaryColor();
        game.accentColor = engineSettings.getThemeAccentColor();
        
        // Show current settings status
        showCurrentSettings();
        
        // Activate the main menu
        activate();
        
        initialized = true;
        lastUpdate = millis();
        
        api.print("Demo ready! Use D-pad to navigate menus");
        return true;
    }

    void update(const WispInputState& input) override {
        if (!initialized) return;

        uint32_t now = millis();
        float deltaTime = (now - lastUpdate) / 1000.0f;
        lastUpdate = now;

        if (inGameMode) {
            updateDemoGame(input, deltaTime);
        } else {
            // Update current menu panel
            if (currentPanel) {
                currentPanel->update(input);
            }
            
            // Check for game activation
            checkGameActivation();
        }
    }

    void render() override {
        if (!initialized) {
            renderLoadingScreen();
            return;
        }

        if (inGameMode) {
            renderDemoGame();
        } else {
            // Render current menu panel
            if (currentPanel) {
                currentPanel->render();
            }
            
            // Show live settings preview
            renderLivePreview();
        }
    }

    void cleanup() override {
        if (initialized) {
            api.print("Cleaning up menu system...");
            cleanup();
            api.print("✓ Menu system cleaned up");
        }
    }

private:
    void showCurrentSettings() {
        api.print("\n=== CURRENT SETTINGS ===");
        
        // Theme settings
        uint16_t primary = engineSettings.getThemePrimaryColor();
        uint16_t accent = engineSettings.getThemeAccentColor();
        api.print("Theme Primary: 0x" + String(primary, HEX));
        api.print("Theme Accent: 0x" + String(accent, HEX));
        
        // Network settings
        bool wifiEnabled = engineSettings.getWiFiEnabled();
        api.print("WiFi Enabled: " + String(wifiEnabled ? "YES" : "NO"));
        
        // Bluetooth settings
        bool btEnabled = engineSettings.getBluetoothEnabled();
        String btName = engineSettings.getBluetoothDeviceName();
        api.print("Bluetooth Enabled: " + String(btEnabled ? "YES" : "NO"));
        api.print("Bluetooth Name: " + btName);
        
        api.print("========================\n");
    }

    void renderLoadingScreen() {
        api.drawRect(0, 0, 320, 240, WispColor(10, 15, 25), 10);
        api.drawText("WISP ENGINE", 160, 80, WispColor(100, 150, 255), 3);
        api.drawText("Menu System Demo", 160, 120, WispColor(150, 150, 150), 2);
        api.drawText("Initializing...", 160, 160, WispColor(100, 100, 100), 1);
        
        // Loading animation
        int dots = (millis() / 500) % 4;
        for (int i = 0; i < dots; i++) {
            api.drawText(".", 200 + i * 10, 160, WispColor(100, 100, 100), 1);
        }
    }

    void checkGameActivation() {
        // If we're on main menu and "Play" is selected, we can start demo game
        if (currentPanel && strcmp(currentPanel->getTitle(), "Main Menu") == 0) {
            // This would need to be implemented with a callback system
            // For now, just check if we should activate game mode
        }
    }

    void updateDemoGame(const WispInputState& input, float deltaTime) {
        if (!game.gameActive) {
            // Start game on A press
            if (input.buttonA) {
                game.gameActive = true;
                game.score = 0;
                // Load current theme colors
                game.primaryColor = engineSettings.getThemePrimaryColor();
                game.accentColor = engineSettings.getThemeAccentColor();
            }
            // Exit to menu on B press
            if (input.buttonB) {
                inGameMode = false;
                activate(); // Reactivate menu
            }
            return;
        }

        // Update bouncing ball demo
        game.playerX += game.velocityX;
        game.playerY += game.velocityY;
        
        // Bounce off walls
        if (game.playerX <= 10 || game.playerX >= 310) {
            game.velocityX = -game.velocityX;
            game.score++;
        }
        if (game.playerY <= 10 || game.playerY >= 230) {
            game.velocityY = -game.velocityY;
            game.score++;
        }
        
        // Keep in bounds
        game.playerX = constrain(game.playerX, 10, 310);
        game.playerY = constrain(game.playerY, 10, 230);
        
        // Exit game on B press
        if (input.buttonB) {
            game.gameActive = false;
            inGameMode = false;
            activate(); // Reactivate menu
        }
    }

    void renderDemoGame() {
        // Background using theme colors
        api.drawRect(0, 0, 320, 240, WispColor(20, 30, 40), 10);
        
        if (!game.gameActive) {
            // Game start screen
            api.drawText("DEMO GAME", 160, 80, WispColor(game.primaryColor), 3);
            api.drawText("This demo uses your theme colors!", 160, 120, 
                        WispColor(200, 200, 200), 1);
            api.drawText("A: Start Game  B: Back to Menu", 160, 160, 
                        WispColor(150, 150, 150), 1);
        } else {
            // Active game - bouncing ball
            api.drawCircle(game.playerX, game.playerY, 8, WispColor(game.primaryColor), 4);
            api.drawRect(game.playerX - 4, game.playerY - 4, 8, 8, 
                        WispColor(game.accentColor), 2);
            
            // Score
            api.drawText("Score: " + String(game.score), 20, 20, 
                        WispColor(game.primaryColor), 1);
            
            // Instructions
            api.drawText("B: Back to Menu", 20, 220, WispColor(100, 100, 100), 1);
        }
    }

    void renderLivePreview() {
        // Live preview of current settings at bottom of screen
        api.drawRect(0, 200, 320, 40, WispColor(0, 0, 0, 100), 0);
        
        // Theme preview
        uint16_t primaryColor = engineSettings.getThemePrimaryColor();
        uint16_t accentColor = engineSettings.getThemeAccentColor();
        
        api.drawRect(10, 205, 20, 15, WispColor(primaryColor), 2);
        api.drawRect(35, 205, 20, 15, WispColor(accentColor), 2);
        api.drawText("Theme", 10, 225, WispColor(150, 150, 150), 1);
        
        // Connection status
        bool wifiEnabled = engineSettings.getWiFiEnabled();
        bool btEnabled = engineSettings.getBluetoothEnabled();
        
        const char* wifiIcon = wifiEnabled ? "📶" : "📵";
        const char* btIcon = btEnabled ? "🔵" : "⚫";
        
        api.drawText("WiFi:" + String(wifiEnabled ? "ON" : "OFF"), 80, 210, 
                    WispColor(wifiEnabled ? 0x07E0 : 0x8410), 1);
        api.drawText("BT:" + String(btEnabled ? "ON" : "OFF"), 80, 225, 
                    WispColor(btEnabled ? 0x001F : 0x8410), 1);
        
        // Current panel
        if (currentPanel) {
            api.drawText(currentPanel->getTitle(), 200, 217, WispColor(100, 100, 100), 1);
        }
    }

    float constrain(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
};

// Global app instance
MenuDemoApp app;

void setup() {
    app.init();
}

void loop() {
    WispInputState input = getInput();
    app.update(input);
    app.render();
    delay(16); // ~60 FPS
}
