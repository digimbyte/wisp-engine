// main_menu.h
#pragma once
#include <LovyanGFX.hpp>
#include "../../engine/app/curated_api.h"
#include "../esp32_common.h"
#include <vector>
#include <string>

class MainMenu {
public:
    char availableApps[20][64]; // Max 20 apps, 64 chars each
    int appCount;
    int selectedApp = 0;
    bool inAppSelection = false;
    std::string statusMessage = "";
    uint32_t statusTimeout = 0;
    
    WispCuratedAPI* api;  // Use curated API instead of direct access
    WispInputState lastInput;  // Track last input for edge detection
    
    void init(WispCuratedAPI* wispAPI) {
        api = wispAPI;
        
        // Get available apps from the curated API
        availableApps = api->getAvailableApps();
        if (availableApps.empty()) {
            statusMessage = "No apps found on SD card";
            statusTimeout = api->getTime() + 3000;
        }
    }
    
    void update() {
        if (!inAppSelection || !api) return;
        
        // Handle input through curated API
        const WispInputState& input = api->getInput();
        if (input.up && !lastInput.up) {  // Just pressed up
            selectedApp = (selectedApp - 1 + availableApps.size()) % availableApps.size();
        }
        if (input.down && !lastInput.down) {  // Just pressed down
            selectedApp = (selectedApp + 1) % availableApps.size();
        }
        if (input.buttonA && !lastInput.buttonA) {  // Just pressed A (select)
            loadSelectedApp();
        }
        if (input.buttonB && !lastInput.buttonB) {  // Just pressed B (back)
            inAppSelection = false;
        }
        
        // Store last input state for edge detection
        lastInput = input;
        
        // Clear status message after timeout
        if (statusTimeout > 0 && api->getTime() > statusTimeout) {
            statusMessage = "";
            statusTimeout = 0;
        }
    }
    
    void render(LGFX& display) {
        display.clear(TFT_BLACK);
        display.setTextColor(TFT_WHITE);
        display.setTextSize(2);
        
        // Title
        display.setTextDatum(top_center);
        display.drawString("WISP ENGINE", SCREEN_WIDTH / 2, 20);
        
        if (!inAppSelection) {
            // Main menu
            display.setTextSize(1);
            display.setTextDatum(middle_center);
            display.drawString("Press A to choose app", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20);
            display.drawString("Found " + String(availableApps.size()) + " apps", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
            
            // Check for button press to enter app selection
            if (api) {
                const WispInputState& input = api->getInput();
                if (input.buttonA && !lastInput.buttonA && !availableApps.empty()) {
                    inAppSelection = true;
                }
            }
        } else {
            // App selection menu
            display.setTextSize(1);
            display.setTextDatum(top_left);
            
            int startY = 60;
            int lineHeight = 20;
            
            for (int i = 0; i < availableApps.size(); i++) {
                uint16_t color = (i == selectedApp) ? TFT_YELLOW : TFT_WHITE;
                display.setTextColor(color);
                
                std::string prefix = (i == selectedApp) ? "> " : "  ";
                display.drawString((prefix + availableApps[i]).c_str(), 20, startY + i * lineHeight);
            }
            
            // Instructions
            display.setTextColor(TFT_GRAY);
            display.setTextDatum(bottom_left);
            display.drawString("UP/DOWN: Navigate", 20, SCREEN_HEIGHT - 40);
            display.drawString("A: Load App", 20, SCREEN_HEIGHT - 25);
            display.drawString("B: Return", 20, SCREEN_HEIGHT - 10);
        }
        
        // Status message
        if (!statusMessage.empty()) {
            display.setTextColor(TFT_RED);
            display.setTextDatum(bottom_center);
            display.drawString(statusMessage, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 5);
        }
    }
    
private:
    void loadSelectedApp() {
        if (selectedApp >= 0 && selectedApp < availableApps.size() && api) {
            std::string appName = availableApps[selectedApp];
            statusMessage = "Loading " + appName + "...";
            statusTimeout = api->getTime() + 2000;
            
            // Use curated API for app loading
            bool result = api->requestAppLaunch(appName);
            
            if (result) {
                statusMessage = "App loaded successfully!";
                inAppSelection = false;
                // App transition handled by engine
            } else {
                statusMessage = "Error: Failed to load app";
                statusTimeout = api->getTime() + 3000;
            }
        }
    }
};
