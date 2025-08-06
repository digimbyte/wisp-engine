// audio_engine_class.h - AudioEngine class wrapper for WispEngine::Audio namespace
#pragma once
#include "audio_engine.h"
#include "audio_outputs.h"

// AudioEngine class wrapper for legacy compatibility
class AudioEngine {
private:
    uint8_t preferredOutputs = AUDIO_PIEZO;
    uint32_t preferredSampleRate = 16000;
    
public:
    bool enabled = false;
    
    AudioEngine() = default;
    ~AudioEngine() = default;
    
    // Wrapper methods that delegate to WispEngine::Audio namespace
    void init() {
        WispEngine::Audio::init();
        enabled = true;
    }
    
    void init(uint8_t outputMask, uint32_t sampleRate) {
        // Initialize with specific outputs and sample rate
        WispEngine::Audio::init();
        preferredOutputs = outputMask;
        preferredSampleRate = sampleRate;
        enabled = true;
    }
    
    void shutdown() {
        WispEngine::Audio::shutdown();
        enabled = false;
    }
    
    void update() {
        WispEngine::Audio::update();
    }
    
    void playBGM(const WBGMHeader* header, const uint8_t* data, uint8_t volume = 255) {
        WispEngine::Audio::playBGM(header, data, volume);
    }
    
    void stopBGM() {
        WispEngine::Audio::stopBGM();
    }
    
    void pauseBGM() {
        WispEngine::Audio::pauseBGM();
    }
    
    void resumeBGM() {
        WispEngine::Audio::resumeBGM();
    }
    
    void setBGMVolume(uint8_t volume) {
        WispEngine::Audio::setBGMVolume(volume);
    }
    
    void playSFX(const WSFXHeader* header, const uint8_t* data) {
        WispEngine::Audio::playSFX(header, data);
    }
    
    void stopAllSFX() {
        WispEngine::Audio::stopAllSFX();
    }
    
    void playCry(const WCrySequenceData* sequence) {
        WispEngine::Audio::playCry(sequence);
    }
    
    void stopCry() {
        WispEngine::Audio::stopCry();
    }
    
    void stopAll() {
        WispEngine::Audio::stopAll();
    }
    
    bool isBGMPlaying() {
        return WispEngine::Audio::isBGMPlaying();
    }
    
    bool isCryPlaying() {
        return WispEngine::Audio::isCryPlaying();
    }
    
    size_t getActiveSFXCount() {
        return WispEngine::Audio::getActiveSFXCount();
    }
    
    void setMasterVolume(uint8_t volume) {
        WispEngine::Audio::setMasterVolume(volume);
    }
    
    uint8_t getMasterVolume() {
        return WispEngine::Audio::getMasterVolume();
    }
    
    // Additional methods that might be expected
    bool isInitialized() const {
        // Could track initialization state if needed
        return true;
    }
    
    bool hasCapability(uint8_t outputType) const {
        // Check if the hardware supports the given output type
        return (outputType & AUDIO_HARDWARE_CAPABILITIES) != 0;
    }
    
    uint8_t getAvailableOutputs() const {
        // Return available audio output capabilities based on hardware
        return AUDIO_HARDWARE_CAPABILITIES;
    }
};

// Global AudioEngine instance for compatibility
extern AudioEngine audio;
