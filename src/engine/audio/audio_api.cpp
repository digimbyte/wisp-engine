// audio_api.cpp - Implementation of Wisp Engine Audio System
#include "audio_engine.h"
#include "audio_mixer.h"
#include "bgm_looper.h"
#include "cry_synth_node.h"
#include <memory>
#include <algorithm>

namespace WispEngine {
namespace Audio {

static AudioMixer g_mixer;
static std::unique_ptr<BGMLooper> g_current_bgm;
static std::vector<std::unique_ptr<SFXPlayer>> g_active_sfx;
static std::unique_ptr<CrySynthNode> g_current_cry;

void init() {
    // Initialize the audio mixer and backend
    // This would set up I2S/A2DP hardware, ring buffers, etc.
    g_mixer.init();
    
    // Clear any existing audio sources
    g_current_bgm.reset();
    g_active_sfx.clear();
    g_current_cry.reset();
    
    // Start mixer processing
    g_mixer.start();
}

void shutdown() {
    // Stop all audio sources
    stopAll();
    
    // Shutdown mixer
    g_mixer.shutdown();
}

void update() {
    // Update mixer (handles automatic cleanup of finished audio sources)
    g_mixer.update();
    
    // Clean up finished SFX
    g_active_sfx.erase(
        std::remove_if(g_active_sfx.begin(), g_active_sfx.end(),
            [](const std::unique_ptr<SFXPlayer>& sfx) {
                return sfx->isFinished();
            }),
        g_active_sfx.end()
    );
}

void playBGM(const WBGMHeader* header, const uint8_t* data, uint8_t volume) {
    // Stop current BGM if playing
    stopBGM();
    
    // Validate WBGM format
    if (!AudioFormats::validateWBGM(header)) {
        return; // Invalid format
    }
    
    // Create new BGM source
    g_current_bgm = std::make_unique<BGMLooper>(header, data);
    g_current_bgm->setVolume(volume);
    
    // Register with mixer
    g_mixer.setBGM(g_current_bgm.get());
}

void stopBGM() {
    if (g_current_bgm) {
        g_mixer.setBGM(nullptr);
        g_current_bgm.reset();
    }
}

void pauseBGM() {
    if (g_current_bgm) {
        g_current_bgm->pause();
    }
}

void resumeBGM() {
    if (g_current_bgm) {
        g_current_bgm->resume();
    }
}

void setBGMVolume(uint8_t volume) {
    if (g_current_bgm) {
        g_current_bgm->setVolume(volume);
    }
}

void playSFX(const WSFXHeader* header, const uint8_t* data) {
    // Validate WSFX format
    if (!AudioFormats::validateWSFX(header)) {
        return; // Invalid format
    }
    
    // Create new SFX source
    auto sfx = std::make_unique<SFXPlayer>(header, data);
    
    // Register with mixer
    g_mixer.addSFX(sfx.get());
    
    // Store for lifecycle management
    g_active_sfx.push_back(std::move(sfx));
}

void stopAllSFX() {
    for (auto& sfx : g_active_sfx) {
        g_mixer.removeSFX(sfx.get());
    }
    g_active_sfx.clear();
}

void playCry(const WCrySequenceData* sequence) {
    // Stop current cry if playing
    stopCry();
    
    // Validate WCRY format
    if (!AudioFormats::validateWCry(&sequence->header)) {
        return; // Invalid format
    }
    
    // Create new cry synth
    g_current_cry = std::make_unique<CrySynthNode>(*sequence);
    
    // Register with mixer (this will pause BGM)
    g_mixer.setCry(g_current_cry.get());
}

void stopCry() {
    if (g_current_cry) {
        g_mixer.setCry(nullptr);  // This will resume BGM
        g_current_cry.reset();
    }
}

void stopAll() {
    stopBGM();
    stopAllSFX();
    stopCry();
}

bool isBGMPlaying() {
    return g_current_bgm && !g_current_bgm->isFinished();
}

bool isCryPlaying() {
    return g_current_cry && !g_current_cry->isFinished();
}

size_t getActiveSFXCount() {
    return g_active_sfx.size();
}

void setMasterVolume(uint8_t volume) {
    g_mixer.setMasterVolume(volume);
}

uint8_t getMasterVolume() {
    return g_mixer.getMasterVolume();
}

} // namespace Audio
} // namespace WispEngine
