// src/system/ui/panels/display_settings.h
#pragma once
#include "../../../engine/app/curated_api.h"
#include "menu.h"

namespace WispMenu {

class DisplaySettingsPanel : public MenuPanel {
private:
    struct DisplaySetting {
        String name;
        int* valuePtr;
        int minValue;
        int maxValue;
        int step;
        String unit;
        
        DisplaySetting(const String& n, int* ptr, int minVal, int maxVal, int s = 1, const String& u = "") :
            name(n), valuePtr(ptr), minValue(minVal), maxValue(maxVal), step(s), unit(u) {}
    };
    
    std::vector<DisplaySetting> settings;
    int selectedIndex;
    bool editMode;
    
    // Display configuration
    int brightness;
    int colorProfile;
    int vsyncEnabled;
    int screenSaver;
    
public:
    DisplaySettingsPanel() : MenuPanel("Display Settings"), selectedIndex(0), editMode(false),
                            brightness(255), colorProfile(0), vsyncEnabled(1), screenSaver(10) {}
    
    bool init() override {
        if (!api) return false;
        
        // Build settings list
        settings.clear();
        settings.push_back(DisplaySetting("Brightness", &brightness, 50, 255, 10, "%"));
        settings.push_back(DisplaySetting("Color Profile", &colorProfile, 0, 3, 1));
        settings.push_back(DisplaySetting("VSync", &vsyncEnabled, 0, 1, 1));
        settings.push_back(DisplaySetting("Screen Saver", &screenSaver, 0, 60, 5, "min"));
        
        return true;
    }
    
    void update(const WispInputState& input) override {
        if (!active) return;
        
        static bool upPressed = false;
        static bool downPressed = false;
        static bool leftPressed = false;
        static bool rightPressed = false;
        static bool selectPressed = false;
        static bool backPressed = false;
        
        if (editMode) {
            // In edit mode - adjust values
            if (input.left && !leftPressed) {
                adjustValue(-1);
            }
            leftPressed = input.left;
            
            if (input.right && !rightPressed) {
                adjustValue(1);
            }
            rightPressed = input.right;
            
            if (input.buttonA && !selectPressed) {
                editMode = false;
                applySettings();
            }
            selectPressed = input.buttonA;
            
        } else {
            // Navigation mode
            if (input.up && !upPressed) {
                selectedIndex = max(0, selectedIndex - 1);
            }
            upPressed = input.up;
            
            if (input.down && !downPressed) {
                selectedIndex = min((int)settings.size() - 1, selectedIndex + 1);
            }
            downPressed = input.down;
            
            if (input.buttonA && !selectPressed) {
                editMode = true;
            }
            selectPressed = input.buttonA;
        }
        
        if (input.buttonB && !backPressed) {
            if (editMode) {
                editMode = false;
            } else {
                deactivate();
            }
        }
        backPressed = input.buttonB;
    }
    
    void render() override {
        if (!active) return;
        
        // Dark background
        api->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WispColor(20, 25, 35), 10);
        
        // Title
        api->drawText("Display Settings", 20, 20, WispColor(255, 255, 255), 2);
        
        // Settings list
        int startY = 60;
        int itemHeight = 30;
        
        for (int i = 0; i < (int)settings.size(); i++) {
            const DisplaySetting& setting = settings[i];
            int y = startY + i * itemHeight;
            bool selected = (i == selectedIndex);
            bool editing = (selected && editMode);
            
            // Highlight if selected
            if (selected) {
                WispColor highlightColor = editing ? WispColor(255, 200, 100, 100) : WispColor(100, 150, 255, 100);
                api->drawRect(10, y - 2, SCREEN_WIDTH - 20, itemHeight - 2, highlightColor, 3);
            }
            
            // Setting name
            WispColor nameColor = selected ? WispColor(255, 255, 255) : WispColor(200, 200, 200);
            api->drawText(setting.name, 20, y + 5, nameColor, 1);
            
            // Setting value
            String valueText = String(*setting.valuePtr);
            if (!setting.unit.empty()) {
                if (setting.unit == "%") {
                    int percent = map(*setting.valuePtr, setting.minValue, setting.maxValue, 0, 100);
                    valueText = std::to_string(percent) + "%";
                } else {
                    valueText += " " + setting.unit;
                }
            }
            
            // Special formatting for boolean values
            if (setting.maxValue == 1 && setting.minValue == 0) {
                valueText = (*setting.valuePtr == 1) ? "ON" : "OFF";
            }
            
            // Special formatting for color profile
            if (setting.name == "Color Profile") {
                switch (*setting.valuePtr) {
                    case 0: valueText = "Standard"; break;
                    case 1: valueText = "Vibrant"; break;
                    case 2: valueText = "Warm"; break;
                    case 3: valueText = "Cool"; break;
                }
            }
            
            WispColor valueColor = editing ? WispColor(255, 255, 100) : WispColor(150, 200, 255);
            api->drawText(valueText, SCREEN_WIDTH - 100, y + 5, valueColor, 1);
            
            // Edit indicators
            if (editing) {
                api->drawText("◄", SCREEN_WIDTH - 130, y + 5, WispColor(255, 255, 100), 1);
                api->drawText("►", SCREEN_WIDTH - 30, y + 5, WispColor(255, 255, 100), 1);
            }
        }
        
        // Instructions
        String instructions = editMode ? "← → Adjust   A: Confirm   B: Cancel" : "↑ ↓ Navigate   A: Edit   B: Back";
        api->drawText(instructions, 20, SCREEN_HEIGHT - 25, WispColor(180, 180, 180), 1);
        
        // Preview area
        renderPreview();
    }
    
    void cleanup() override {
        // Save settings before cleanup
        saveSettings();
    }
    
private:
    void adjustValue(int direction) {
        if (selectedIndex < 0 || selectedIndex >= (int)settings.size()) return;
        
        DisplaySetting& setting = settings[selectedIndex];
        int newValue = *setting.valuePtr + (direction * setting.step);
        *setting.valuePtr = constrain(newValue, setting.minValue, setting.maxValue);
        
        // Apply setting immediately for preview
        applySettings();
    }
    
    void applySettings() {
        // Apply brightness immediately
        // TODO: This would interface with the actual display system
        api->print("Applied brightness: " + std::to_string(brightness));
        
        // Apply color profile
        api->print("Applied color profile: " + std::to_string(colorProfile));
        
        // Apply VSync setting
        api->print("Applied VSync: " + String(vsyncEnabled ? "ON" : "OFF"));
    }
    
    void saveSettings() {
        // Save to persistent storage
        api->saveData("display.brightness", std::to_string(brightness));
        api->saveData("display.colorProfile", std::to_string(colorProfile));
        api->saveData("display.vsync", std::to_string(vsyncEnabled));
        api->saveData("display.screenSaver", std::to_string(screenSaver));
        
        api->print("Display settings saved");
    }
    
    void loadSettings() {
        // Load from persistent storage
        brightness = api->loadData("display.brightness", "255").toInt();
        colorProfile = api->loadData("display.colorProfile", "0").toInt();
        vsyncEnabled = api->loadData("display.vsync", "1").toInt();
        screenSaver = api->loadData("display.screenSaver", "10").toInt();
        
        // Validate ranges
        brightness = constrain(brightness, 50, 255);
        colorProfile = constrain(colorProfile, 0, 3);
        vsyncEnabled = constrain(vsyncEnabled, 0, 1);
        screenSaver = constrain(screenSaver, 0, 60);
    }
    
    void renderPreview() {
        // Show a preview of the current settings
        int previewX = SCREEN_WIDTH - 80;
        int previewY = 60;
        int previewSize = 60;
        
        // Preview background
        api->drawRect(previewX, previewY, previewSize, previewSize, WispColor(50, 60, 80), 4);
        
        // Brightness preview
        uint8_t previewBrightness = map(brightness, 50, 255, 50, 255);
        WispColor brightColor(previewBrightness, previewBrightness, previewBrightness);
        api->drawRect(previewX + 10, previewY + 10, 20, 20, brightColor, 3);
        
        // Color profile preview
        WispColor profileColor;
        switch (colorProfile) {
            case 0: profileColor = WispColor(255, 255, 255); break; // Standard
            case 1: profileColor = WispColor(255, 100, 100); break; // Vibrant
            case 2: profileColor = WispColor(255, 200, 150); break; // Warm
            case 3: profileColor = WispColor(150, 200, 255); break; // Cool
        }
        api->drawRect(previewX + 35, previewY + 10, 20, 20, profileColor, 3);
        
        // VSync indicator
        if (vsyncEnabled) {
            api->drawRect(previewX + 10, previewY + 35, 45, 5, WispColor(100, 255, 100), 2);
        } else {
            api->drawRect(previewX + 10, previewY + 35, 45, 5, WispColor(255, 100, 100), 2);
        }
    }
};

} // namespace WispMenu
