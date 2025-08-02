// audio_settings.h - Audio Settings Panel for Wisp Engine
#ifndef AUDIO_SETTINGS_H
#define AUDIO_SETTINGS_H

#include "menu.h"
#include "../../system/definitions.h"

class AudioSettingsPanel : public MenuPanel {
private:
    struct AudioSettings {
        uint8_t masterVolume = 80;      // 0-100
        uint8_t effectsVolume = 75;     // 0-100
        uint8_t musicVolume = 70;       // 0-100
        bool enablePiezo = true;        // Piezo buzzer enabled
        bool enableI2S = false;         // I2S audio output (if available)
        uint8_t toneQuality = 2;        // 0=low, 1=medium, 2=high
        bool enableHaptics = true;      // Vibration feedback
        uint8_t hapticStrength = 50;    // 0-100
    } settings;
    
    enum AudioMenuState {
        MASTER_VOLUME,
        EFFECTS_VOLUME,
        MUSIC_VOLUME,
        OUTPUT_MODE,
        TONE_QUALITY,
        HAPTIC_FEEDBACK,
        HAPTIC_STRENGTH,
        TEST_AUDIO,
        SAVE_SETTINGS,
        AUDIO_MENU_COUNT
    };
    
    AudioMenuState currentSelection = MASTER_VOLUME;
    bool inAdjustMode = false;
    uint32_t lastTestTime = 0;
    
    const char* menuItems[AUDIO_MENU_COUNT] = {
        "Master Volume",
        "Effects Volume", 
        "Music Volume",
        "Output Mode",
        "Tone Quality",
        "Haptic Feedback",
        "Haptic Strength",
        "Test Audio",
        "Save & Exit"
    };

public:
    AudioSettingsPanel(WispCuratedAPI* api) : MenuPanel(api) {
        loadSettings();
    }
    
    void activate() override {
        MenuPanel::activate();
        currentSelection = MASTER_VOLUME;
        inAdjustMode = false;
        loadSettings();
    }
    
    void update(const WispInputState& input) override {
        if (!isActive()) return;
        
        if (inAdjustMode) {
            handleAdjustment(input);
        } else {
            handleNavigation(input);
        }
        
        // Auto-save periodically
        static uint32_t lastSaveTime = 0;
        if (millis() - lastSaveTime > 10000) { // Every 10 seconds
            saveSettings();
            lastSaveTime = millis();
        }
    }
    
    void render() override {
        if (!isActive()) return;
        
        auto* gfx = api->graphics();
        
        // Clear background
        gfx->fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_DARK_BLUE);
        
        // Title
        gfx->setTextColor(COLOR_WHITE);
        gfx->setTextSize(2);
        gfx->drawText("AUDIO SETTINGS", SCREEN_WIDTH / 2, 15, true);
        
        // Menu items
        gfx->setTextSize(1);
        const int startY = 45;
        const int itemHeight = 20;
        
        for (int i = 0; i < AUDIO_MENU_COUNT; i++) {
            int y = startY + i * itemHeight;
            
            // Highlight current selection
            if (i == currentSelection) {
                uint16_t highlightColor = inAdjustMode ? COLOR_ORANGE : COLOR_LIGHT_BLUE;
                gfx->fillRect(5, y - 2, SCREEN_WIDTH - 10, itemHeight - 2, highlightColor);
                gfx->setTextColor(COLOR_BLACK);
            } else {
                gfx->setTextColor(COLOR_WHITE);
            }
            
            // Draw menu item and value
            gfx->drawText(menuItems[i], 10, y + 5, false);
            
            String valueText = getValueText((AudioMenuState)i);
            if (!valueText.isEmpty()) {
                gfx->drawText(valueText.c_str(), SCREEN_WIDTH - 10, y + 5, false, true);
            }
        }
        
        // Instructions
        gfx->setTextColor(COLOR_LIGHT_GRAY);
        gfx->setTextSize(1);
        if (inAdjustMode) {
            gfx->drawText("LEFT/RIGHT: Adjust | SELECT: Confirm", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 15, true);
        } else {
            gfx->drawText("UP/DOWN: Navigate | SELECT: Adjust | BACK: Exit", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 15, true);
        }
        
        // Real-time audio visualization
        renderAudioVisualizer();
    }
    
private:
    void handleNavigation(const WispInputState& input) {
        static uint32_t lastInputTime = 0;
        uint32_t currentTime = millis();
        
        if (currentTime - lastInputTime < 150) return; // Debounce
        
        if (input.up) {
            currentSelection = (AudioMenuState)((currentSelection - 1 + AUDIO_MENU_COUNT) % AUDIO_MENU_COUNT);
            playNavigationSound();
            lastInputTime = currentTime;
        } else if (input.down) {
            currentSelection = (AudioMenuState)((currentSelection + 1) % AUDIO_MENU_COUNT);
            playNavigationSound();
            lastInputTime = currentTime;
        } else if (input.buttonA || input.select) {
            if (currentSelection == SAVE_SETTINGS) {
                saveSettings();
                deactivate();
            } else if (currentSelection == TEST_AUDIO) {
                testAudio();
            } else {
                inAdjustMode = true;
                playConfirmSound();
            }
            lastInputTime = currentTime;
        } else if (input.buttonB) {
            deactivate();
            lastInputTime = currentTime;
        }
    }
    
    void handleAdjustment(const WispInputState& input) {
        static uint32_t lastInputTime = 0;
        uint32_t currentTime = millis();
        
        if (currentTime - lastInputTime < 100) return; // Faster adjustment
        
        bool changed = false;
        
        if (input.left) {
            changed = adjustValue(currentSelection, -1);
            lastInputTime = currentTime;
        } else if (input.right) {
            changed = adjustValue(currentSelection, 1);
            lastInputTime = currentTime;
        } else if (input.buttonA || input.select) {
            inAdjustMode = false;
            playConfirmSound();
            lastInputTime = currentTime;
        } else if (input.buttonB) {
            inAdjustMode = false;
            lastInputTime = currentTime;
        }
        
        if (changed) {
            playAdjustmentSound();
            // Immediate feedback for volume changes
            if (currentSelection == MASTER_VOLUME || currentSelection == EFFECTS_VOLUME) {
                testVolumeLevel();
            }
        }
    }
    
    bool adjustValue(AudioMenuState item, int delta) {
        switch (item) {
            case MASTER_VOLUME:
                settings.masterVolume = constrain(settings.masterVolume + delta * 5, 0, 100);
                return true;
                
            case EFFECTS_VOLUME:
                settings.effectsVolume = constrain(settings.effectsVolume + delta * 5, 0, 100);
                return true;
                
            case MUSIC_VOLUME:
                settings.musicVolume = constrain(settings.musicVolume + delta * 5, 0, 100);
                return true;
                
            case OUTPUT_MODE:
                if (delta > 0) {
                    if (settings.enablePiezo && !settings.enableI2S) {
                        settings.enableI2S = true; // Piezo + I2S
                    } else if (settings.enableI2S) {
                        settings.enablePiezo = false; // I2S only
                    } else {
                        settings.enablePiezo = true;
                        settings.enableI2S = false; // Back to Piezo only
                    }
                } else {
                    if (!settings.enablePiezo && settings.enableI2S) {
                        settings.enablePiezo = true; // I2S only -> Piezo + I2S
                    } else if (settings.enablePiezo && settings.enableI2S) {
                        settings.enableI2S = false; // Piezo + I2S -> Piezo only
                    } else {
                        settings.enableI2S = true;
                        settings.enablePiezo = false; // Piezo only -> I2S only
                    }
                }
                return true;
                
            case TONE_QUALITY:
                settings.toneQuality = constrain(settings.toneQuality + delta, 0, 2);
                return true;
                
            case HAPTIC_FEEDBACK:
                settings.enableHaptics = !settings.enableHaptics;
                return true;
                
            case HAPTIC_STRENGTH:
                settings.hapticStrength = constrain(settings.hapticStrength + delta * 10, 0, 100);
                return true;
                
            default:
                return false;
        }
    }
    
    String getValueText(AudioMenuState item) {
        switch (item) {
            case MASTER_VOLUME:
                return String(settings.masterVolume) + "%";
                
            case EFFECTS_VOLUME:
                return String(settings.effectsVolume) + "%";
                
            case MUSIC_VOLUME:
                return String(settings.musicVolume) + "%";
                
            case OUTPUT_MODE:
                if (settings.enablePiezo && settings.enableI2S) return "Piezo+I2S";
                else if (settings.enableI2S) return "I2S Only";
                else return "Piezo Only";
                
            case TONE_QUALITY:
                switch (settings.toneQuality) {
                    case 0: return "Low";
                    case 1: return "Medium";
                    case 2: return "High";
                    default: return "Unknown";
                }
                
            case HAPTIC_FEEDBACK:
                return settings.enableHaptics ? "Enabled" : "Disabled";
                
            case HAPTIC_STRENGTH:
                return settings.enableHaptics ? (String(settings.hapticStrength) + "%") : "N/A";
                
            default:
                return "";
        }
    }
    
    void renderAudioVisualizer() {
        // Simple audio level indicator
        auto* gfx = api->graphics();
        
        const int barX = SCREEN_WIDTH - 40;
        const int barY = 50;
        const int barWidth = 20;
        const int barHeight = 80;
        
        // Background
        gfx->drawRect(barX, barY, barWidth, barHeight, COLOR_WHITE);
        
        // Level indicator based on master volume
        int levelHeight = (settings.masterVolume * barHeight) / 100;
        uint16_t levelColor = COLOR_GREEN;
        if (settings.masterVolume > 80) levelColor = COLOR_ORANGE;
        if (settings.masterVolume > 95) levelColor = COLOR_RED;
        
        gfx->fillRect(barX + 1, barY + barHeight - levelHeight, barWidth - 2, levelHeight, levelColor);
        
        // Volume percentage
        gfx->setTextColor(COLOR_WHITE);
        gfx->setTextSize(1);
        gfx->drawText(String(settings.masterVolume).c_str(), barX + barWidth/2, barY + barHeight + 5, true);
    }
    
    void testAudio() {
        if (millis() - lastTestTime < 1000) return; // Prevent spam
        
        // Play test sounds using current settings
        api->audio()->playTone(440, 200, settings.effectsVolume); // A4 note
        
        if (settings.enableHaptics) {
            // Trigger haptic feedback
            // This would need to be implemented in the curated API
        }
        
        lastTestTime = millis();
    }
    
    void testVolumeLevel() {
        // Quick volume test sound
        api->audio()->playTone(880, 100, settings.effectsVolume); // Higher pitch, shorter
    }
    
    void playNavigationSound() {
        api->audio()->playTone(220, 50, settings.effectsVolume * 0.3f);
    }
    
    void playConfirmSound() {
        api->audio()->playTone(440, 100, settings.effectsVolume * 0.5f);
    }
    
    void playAdjustmentSound() {
        api->audio()->playTone(330, 30, settings.effectsVolume * 0.2f);
    }
    
    void loadSettings() {
        // Load from persistent storage
        // This would use SPIFFS or preferences library
        // For now, use defaults
        
        // Example using Preferences:
        // Preferences prefs;
        // prefs.begin("audio", false);
        // settings.masterVolume = prefs.getUChar("masterVol", 80);
        // settings.effectsVolume = prefs.getUChar("effectsVol", 75);
        // settings.musicVolume = prefs.getUChar("musicVol", 70);
        // settings.enablePiezo = prefs.getBool("piezo", true);
        // settings.enableI2S = prefs.getBool("i2s", false);
        // settings.toneQuality = prefs.getUChar("quality", 2);
        // settings.enableHaptics = prefs.getBool("haptics", true);
        // settings.hapticStrength = prefs.getUChar("hapticStr", 50);
        // prefs.end();
    }
    
    void saveSettings() {
        // Save to persistent storage
        // Example using Preferences:
        // Preferences prefs;
        // prefs.begin("audio", false);
        // prefs.putUChar("masterVol", settings.masterVolume);
        // prefs.putUChar("effectsVol", settings.effectsVolume);
        // prefs.putUChar("musicVol", settings.musicVolume);
        // prefs.putBool("piezo", settings.enablePiezo);
        // prefs.putBool("i2s", settings.enableI2S);
        // prefs.putUChar("quality", settings.toneQuality);
        // prefs.putBool("haptics", settings.enableHaptics);
        // prefs.putUChar("hapticStr", settings.hapticStrength);
        // prefs.end();
        
        // Apply settings to audio system
        applyAudioSettings();
    }
    
    void applyAudioSettings() {
        // Apply current settings to the audio system
        // This would call into the curated API to update audio configuration
        api->audio()->setMasterVolume(settings.masterVolume);
        api->audio()->setEffectsVolume(settings.effectsVolume);
        api->audio()->setMusicVolume(settings.musicVolume);
        
        // Configure output modes
        // api->audio()->enablePiezo(settings.enablePiezo);
        // api->audio()->enableI2S(settings.enableI2S);
        // api->audio()->setToneQuality(settings.toneQuality);
        
        // Configure haptics
        // api->haptics()->setEnabled(settings.enableHaptics);
        // api->haptics()->setStrength(settings.hapticStrength);
    }
};

#endif // AUDIO_SETTINGS_H
