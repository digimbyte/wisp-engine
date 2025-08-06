// examples/audio_test_app.cpp - Audio System Test
// Tests BGM, SFX, cry synthesis, and audio mixing capabilities

#include "../src/engine/app/interface.h"

class AudioTestApp : public WispAppBase {
private:
    // Audio test modes
    enum AudioTestMode {
        TEST_BGM,           // Background music test
        TEST_SFX,           // Sound effects test  
        TEST_CRY,           // Cry synthesis test
        TEST_MIXING,        // Multi-channel mixing test
        TEST_COUNT
    };
    
    AudioTestMode currentMode = TEST_BGM;
    
    // Audio resources
    ResourceHandle bgmTracks[4];
    ResourceHandle sfxSounds[8];
    ResourceHandle crySamples[6];
    
    uint8_t bgmCount = 0;
    uint8_t sfxCount = 0;
    uint8_t cryCount = 0;
    
    // Playback state
    uint8_t currentBGM = 0;
    uint8_t masterVolume = 255;
    uint8_t bgmVolume = 200;
    bool bgmPlaying = false;
    
    // SFX test state
    uint32_t lastSFXTime = 0;
    uint8_t sfxInterval = 1000; // ms
    bool autoSFX = false;
    
    // Mixing test
    uint8_t activeSFXChannels = 0;
    uint32_t mixingTestStart = 0;
    
    // Audio visualizer (simple)
    float audioLevels[8] = {0}; // Simulated audio levels for visualization

public:
    bool init() override {
        setAppInfo("Audio Test", "1.0.0", "Wisp Engine Team");
        
        // Load test audio assets
        loadAudioAssets();
        
        api->print("Audio Test App initialized");
        api->print("Controls: Up/Down - Mode, A - Play/Stop, B - Next Track");
        api->print("Left/Right - Volume, Start - Auto SFX");
        return true;
    }
    
    void loadAudioAssets() {
        // Load BGM tracks from assets folder
        bgmTracks[0] = api->loadAudio("assets/test_bgm_calm.wbgm");
        bgmTracks[1] = api->loadAudio("assets/test_bgm_action.wbgm");
        bgmTracks[2] = api->loadAudio("assets/test_bgm_ambient.wbgm");
        bgmTracks[3] = api->loadAudio("assets/test_bgm_battle.wbgm");
        bgmCount = 4;
        
        // Load SFX sounds from assets folder
        sfxSounds[0] = api->loadAudio("assets/test_sfx_beep.wsfx");
        sfxSounds[1] = api->loadAudio("assets/test_sfx_explosion.wsfx");
        sfxSounds[2] = api->loadAudio("assets/test_sfx_pickup.wsfx");
        sfxSounds[3] = api->loadAudio("assets/test_sfx_jump.wsfx");
        sfxSounds[4] = api->loadAudio("assets/test_sfx_hit.wsfx");
        sfxSounds[5] = api->loadAudio("assets/test_sfx_powerup.wsfx");
        sfxSounds[6] = api->loadAudio("assets/test_sfx_menu.wsfx");
        sfxSounds[7] = api->loadAudio("assets/test_sfx_error.wsfx");
        sfxCount = 8;
        
        // Load cry samples from assets folder
        crySamples[0] = api->loadAudio("assets/test_cry_pikachu.wcry");
        crySamples[1] = api->loadAudio("assets/test_cry_charizard.wcry");
        crySamples[2] = api->loadAudio("assets/test_cry_blastoise.wcry");
        crySamples[3] = api->loadAudio("assets/test_cry_venusaur.wcry");
        crySamples[4] = api->loadAudio("assets/test_cry_mewtwo.wcry");
        crySamples[5] = api->loadAudio("assets/test_cry_mew.wcry");
        cryCount = 6;
    }
    
    std::string getModeName(AudioTestMode mode) {
        const char* names[] = {"BGM Test", "SFX Test", "Cry Test", "Mixing Test"};
        return names[mode];
    }
    
    std::string getBGMName(uint8_t index) {
        const char* names[] = {"Calm Theme", "Action Theme", "Ambient Theme", "Battle Theme"};
        return names[index % bgmCount];
    }
    
    std::string getSFXName(uint8_t index) {
        const char* names[] = {"Beep", "Explosion", "Pickup", "Jump", "Hit", "PowerUp", "Menu", "Error"};
        return names[index % sfxCount];
    }
    
    std::string getCryName(uint8_t index) {
        const char* names[] = {"Pikachu", "Charizard", "Blastoise", "Venusaur", "Mewtwo", "Mew"};
        return names[index % cryCount];
    }
    
    void update() override {
        uint32_t currentTime = api->getTime();
        
        // Handle input
        const WispInputState& input = api->getInput();
        static WispInputState lastInput;
        
        // Mode selection
        if (input.up && !lastInput.up) {
            currentMode = (AudioTestMode)((currentMode + 1) % TEST_COUNT);
            api->print("Audio Mode: " + getModeName(currentMode));
        }
        if (input.down && !lastInput.down) {
            currentMode = (AudioTestMode)((currentMode - 1 + TEST_COUNT) % TEST_COUNT);
            api->print("Audio Mode: " + getModeName(currentMode));
        }
        
        // Volume control
        if (input.left && !lastInput.left) {
            masterVolume = std::max(0, masterVolume - 25);
            // api->setMasterVolume(masterVolume); // Would call actual audio API
            api->print("Master Volume: " + std::to_string(masterVolume));
        }
        if (input.right && !lastInput.right) {
            masterVolume = std::min(255, masterVolume + 25);
            // api->setMasterVolume(masterVolume); // Would call actual audio API
            api->print("Master Volume: " + std::to_string(masterVolume));
        }
        
        // Mode-specific controls
        switch (currentMode) {
            case TEST_BGM:
                handleBGMControls(input, lastInput);
                break;
            case TEST_SFX:
                handleSFXControls(input, lastInput, currentTime);
                break;
            case TEST_CRY:
                handleCryControls(input, lastInput);
                break;
            case TEST_MIXING:
                handleMixingControls(input, lastInput, currentTime);
                break;
        }
        
        lastInput = input;
        
        // Update audio visualizer (simulate audio levels)
        updateAudioVisualizer(currentTime);
    }
    
    void handleBGMControls(const WispInputState& input, const WispInputState& lastInput) {
        if (input.buttonA && !lastInput.buttonA) {
            if (bgmPlaying) {
                // Stop BGM
                api->stopAudio(bgmTracks[currentBGM]);
                bgmPlaying = false;
                api->print("BGM Stopped");
            } else {
                // Play BGM
                WispAudioParams params;
                params.volume = bgmVolume / 255.0f;
                params.loop = true;
                
                if (api->playAudio(bgmTracks[currentBGM], params)) {
                    bgmPlaying = true;
                    api->print("Playing: " + getBGMName(currentBGM));
                } else {
                    api->print("Failed to play BGM");
                }
            }
        }
        
        if (input.buttonB && !lastInput.buttonB) {
            // Next BGM track
            if (bgmPlaying) {
                api->stopAudio(bgmTracks[currentBGM]);
            }
            currentBGM = (currentBGM + 1) % bgmCount;
            api->print("Selected: " + getBGMName(currentBGM));
        }
    }
    
    void handleSFXControls(const WispInputState& input, const WispInputState& lastInput, uint32_t currentTime) {
        if (input.buttonA && !lastInput.buttonA) {
            // Play random SFX
            uint8_t sfxIndex = api->randomInt(0, sfxCount - 1);
            
            WispAudioParams params;
            params.volume = 0.8f;
            params.loop = false;
            
            if (api->playAudio(sfxSounds[sfxIndex], params)) {
                api->print("Playing SFX: " + getSFXName(sfxIndex));
            }
        }
        
        if (input.buttonB && !lastInput.buttonB) {
            autoSFX = !autoSFX;
            api->print("Auto SFX: " + std::string(autoSFX ? "ON" : "OFF"));
        }
        
        // Auto SFX playback
        if (autoSFX && currentTime - lastSFXTime > sfxInterval) {
            uint8_t sfxIndex = api->randomInt(0, sfxCount - 1);
            
            WispAudioParams params;
            params.volume = 0.6f;
            params.loop = false;
            
            api->playAudio(sfxSounds[sfxIndex], params);
            lastSFXTime = currentTime;
        }
    }
    
    void handleCryControls(const WispInputState& input, const WispInputState& lastInput) {
        if (input.buttonA && !lastInput.buttonA) {
            // Play random cry
            uint8_t cryIndex = api->randomInt(0, cryCount - 1);
            
            WispAudioParams params;
            params.volume = 0.9f;
            params.loop = false;
            
            if (api->playAudio(crySamples[cryIndex], params)) {
                api->print("Playing Cry: " + getCryName(cryIndex));
            }
        }
        
        if (input.buttonB && !lastInput.buttonB) {
            // Stop current cry
            // api->stopAllCries(); // Would call actual audio API
            api->print("All cries stopped");
        }
    }
    
    void handleMixingControls(const WispInputState& input, const WispInputState& lastInput, uint32_t currentTime) {
        if (input.buttonA && !lastInput.buttonA) {
            // Start mixing stress test
            mixingTestStart = currentTime;
            activeSFXChannels = 0;
            
            // Play multiple SFX simultaneously
            for (int i = 0; i < 4; i++) {
                WispAudioParams params;
                params.volume = 0.4f;
                params.loop = false;
                
                if (api->playAudio(sfxSounds[i], params)) {
                    activeSFXChannels++;
                }
            }
            
            api->print("Mixing test started - " + std::to_string(activeSFXChannels) + " channels");
        }
        
        if (input.buttonB && !lastInput.buttonB) {
            // Stop all audio
            for (int i = 0; i < sfxCount; i++) {
                api->stopAudio(sfxSounds[i]);
            }
            activeSFXChannels = 0;
            api->print("All audio stopped");
        }
    }
    
    void updateAudioVisualizer(uint32_t currentTime) {
        // Simulate audio levels for visualization
        for (int i = 0; i < 8; i++) {
            float decay = 0.95f;
            audioLevels[i] *= decay;
            
            // Add random spikes when audio is "playing"
            if (bgmPlaying || activeSFXChannels > 0) {
                if (api->randomInt(0, 10) == 0) {
                    audioLevels[i] = api->random(0.3f, 1.0f);
                }
            }
        }
    }
    
    void render() override {
        // Clear with dark background
        api->drawRect(0, 0, 320, 240, WispColor(15, 15, 30), 0);
        
        // Draw title
        api->drawText("AUDIO TEST", 160, 10, WispColor(255, 255, 255), 10);
        
        // Draw current mode
        api->drawText(getModeName(currentMode), 160, 25, WispColor(200, 200, 255), 9);
        
        // Draw mode-specific UI
        switch (currentMode) {
            case TEST_BGM:
                renderBGMTest();
                break;
            case TEST_SFX:
                renderSFXTest();
                break;
            case TEST_CRY:
                renderCryTest();
                break;
            case TEST_MIXING:
                renderMixingTest();
                break;
        }
        
        // Draw audio visualizer
        renderAudioVisualizer();
        
        // Draw controls
        api->drawText("Up/Down: Mode  A: Play/Stop  B: Next/Auto", 10, 210, WispColor(180, 180, 180), 8);
        api->drawText("Left/Right: Volume", 10, 225, WispColor(180, 180, 180), 8);
        
        // Draw master volume
        std::string volumeText = "Master Volume: " + std::to_string(masterVolume);
        api->drawText(volumeText, 250, 210, WispColor(255, 255, 255), 8);
    }
    
    void renderBGMTest() {
        api->drawText("Current Track: " + getBGMName(currentBGM), 50, 50, WispColor(255, 255, 255), 8);
        
        std::string status = bgmPlaying ? "PLAYING" : "STOPPED";
        WispColor statusColor = bgmPlaying ? WispColor(0, 255, 0) : WispColor(255, 0, 0);
        api->drawText("Status: " + status, 50, 65, statusColor, 8);
        
        api->drawText("BGM Volume: " + std::to_string(bgmVolume), 50, 80, WispColor(200, 200, 200), 8);
    }
    
    void renderSFXTest() {
        api->drawText("SFX Channels Available: 4", 50, 50, WispColor(255, 255, 255), 8);
        
        std::string autoStatus = autoSFX ? "ON" : "OFF";
        WispColor autoColor = autoSFX ? WispColor(0, 255, 0) : WispColor(255, 0, 0);
        api->drawText("Auto SFX: " + autoStatus, 50, 65, autoColor, 8);
        
        api->drawText("Interval: " + std::to_string(sfxInterval) + "ms", 50, 80, WispColor(200, 200, 200), 8);
    }
    
    void renderCryTest() {
        api->drawText("Cry Synthesis Engine", 50, 50, WispColor(255, 255, 255), 8);
        api->drawText("Supports procedural sound generation", 50, 65, WispColor(200, 200, 200), 8);
        api->drawText("Available Cries: " + std::to_string(cryCount), 50, 80, WispColor(200, 200, 200), 8);
    }
    
    void renderMixingTest() {
        api->drawText("Multi-channel Audio Mixing", 50, 50, WispColor(255, 255, 255), 8);
        api->drawText("Active SFX Channels: " + std::to_string(activeSFXChannels), 50, 65, WispColor(200, 200, 200), 8);
        
        if (mixingTestStart > 0) {
            uint32_t elapsed = (api->getTime() - mixingTestStart) / 1000;
            api->drawText("Test Running: " + std::to_string(elapsed) + "s", 50, 80, WispColor(255, 255, 0), 8);
        }
    }
    
    void renderAudioVisualizer() {
        // Simple audio level bars
        int startX = 50;
        int startY = 120;
        int barWidth = 20;
        int maxHeight = 60;
        
        api->drawText("Audio Levels:", startX, startY - 15, WispColor(200, 200, 200), 8);
        
        for (int i = 0; i < 8; i++) {
            int barHeight = (int)(audioLevels[i] * maxHeight);
            int x = startX + i * (barWidth + 2);
            
            // Background bar
            api->drawRect(x, startY, barWidth, maxHeight, WispColor(40, 40, 40), 3);
            
            // Active level bar
            if (barHeight > 0) {
                WispColor levelColor;
                if (audioLevels[i] > 0.8f) levelColor = WispColor(255, 0, 0);      // Red - high
                else if (audioLevels[i] > 0.5f) levelColor = WispColor(255, 255, 0); // Yellow - med
                else levelColor = WispColor(0, 255, 0);                             // Green - low
                
                api->drawRect(x, startY + maxHeight - barHeight, barWidth, barHeight, levelColor, 4);
            }
            
            // Channel number
            api->drawText(std::to_string(i), x + 6, startY + maxHeight + 5, WispColor(150, 150, 150), 8);
        }
    }
    
    void cleanup() override {
        // Unload all audio resources
        for (int i = 0; i < bgmCount; i++) {
            api->unloadAudio(bgmTracks[i]);
        }
        for (int i = 0; i < sfxCount; i++) {
            api->unloadAudio(sfxSounds[i]);
        }
        for (int i = 0; i < cryCount; i++) {
            api->unloadAudio(crySamples[i]);
        }
        
        api->print("Audio Test App cleaned up");
    }
};

// Export function for the engine
extern "C" WispAppBase* createAudioTestApp() {
    return new AudioTestApp();
}

extern "C" void destroyAudioTestApp(WispAppBase* app) {
    delete app;
}
