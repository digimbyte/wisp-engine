// audio_mixer.cpp - Priority-based audio mixer implementation
#include "audio_mixer.h"
#include <algorithm>
#include <cstring>

AudioMixer::AudioMixer() 
    : rawBgmPointer(nullptr), rawCryPointer(nullptr),
      bgmPaused(false), fadingOutBGM(false), fadeCounter(0), 
      masterVolume(255), initialized(false) {
    memset(tempBuffer, 0, sizeof(tempBuffer));
}

AudioMixer::~AudioMixer() {
    shutdown();
}

void AudioMixer::init() {
    if (initialized) return;
    
    // Initialize audio hardware (I2S, A2DP, etc.)
    // This would be platform-specific implementation
    masterVolume = 255;
    initialized = true;
}

void AudioMixer::start() {
    if (!initialized) {
        init();
    }
    // Start audio processing thread/timer
    // Platform-specific implementation
}

void AudioMixer::shutdown() {
    if (!initialized) return;
    
    stopAll();
    // Shutdown audio hardware
    // Platform-specific implementation
    initialized = false;
}

void AudioMixer::mix(int16_t* buffer, size_t samples) {
    if (!initialized) return;
    
    // Clear output buffer
    memset(buffer, 0, samples * sizeof(int16_t));
    
    // Handle cry with exclusive priority
    if (cry) {
        cry->render(buffer, samples);
        
        // Check if cry finished
        if (cry->isFinished()) {
            cry.reset();
            rawCryPointer = nullptr;
            // Resume BGM if it was paused
            if (bgmPaused && bgm) {
                bgm->resume();
                bgmPaused = false;
            }
        }
        
        // Apply master volume
        for (size_t i = 0; i < samples; i++) {
            int32_t sample = buffer[i] * masterVolume / 255;
            buffer[i] = softClip(sample);
        }
        return; // Cry has exclusive focus - no mixing with other sources
    }
    
    // Handle raw cry pointer
    if (rawCryPointer) {
        rawCryPointer->render(buffer, samples);
        
        if (rawCryPointer->isFinished()) {
            rawCryPointer = nullptr;
            // Resume BGM if it was paused
            if (bgmPaused && (bgm || rawBgmPointer)) {
                if (bgm) bgm->resume();
                if (rawBgmPointer) rawBgmPointer->resume();
                bgmPaused = false;
            }
        }
        
        // Apply master volume
        for (size_t i = 0; i < samples; i++) {
            int32_t sample = buffer[i] * masterVolume / 255;
            buffer[i] = softClip(sample);
        }
        return; // Cry has exclusive focus
    }
    
    // Render BGM (if not paused)
    if (bgm && !bgmPaused) {
        bgm->render(buffer, samples);
    } else if (rawBgmPointer && !bgmPaused) {
        rawBgmPointer->render(buffer, samples);
    }
    
    // Mix SFX additively with BGM
    cleanupFinishedSFX();
    for (auto& sfx : sfxQueue) {
        if (sfx && !sfx->isFinished()) {
            // Render SFX to temp buffer
            memset(tempBuffer, 0, std::min(samples, (size_t)AUDIO_BUFFER_SIZE) * sizeof(int16_t));
            sfx->render(tempBuffer, std::min(samples, (size_t)AUDIO_BUFFER_SIZE));
            
            // Mix with output buffer
            for (size_t i = 0; i < std::min(samples, (size_t)AUDIO_BUFFER_SIZE); i++) {
                int32_t mixed = buffer[i] + tempBuffer[i];
                buffer[i] = softClip(mixed);
            }
        }
    }
    
    // Mix raw SFX pointers
    for (auto* sfx : rawSfxPointers) {
        if (sfx && !sfx->isFinished()) {
            // Render SFX to temp buffer
            memset(tempBuffer, 0, std::min(samples, (size_t)AUDIO_BUFFER_SIZE) * sizeof(int16_t));
            sfx->render(tempBuffer, std::min(samples, (size_t)AUDIO_BUFFER_SIZE));
            
            // Mix with output buffer
            for (size_t i = 0; i < std::min(samples, (size_t)AUDIO_BUFFER_SIZE); i++) {
                int32_t mixed = buffer[i] + tempBuffer[i];
                buffer[i] = softClip(mixed);
            }
        }
    }
    
    // Apply master volume
    for (size_t i = 0; i < samples; i++) {
        int32_t sample = buffer[i] * masterVolume / 255;
        buffer[i] = softClip(sample);
    }
}

void AudioMixer::update() {
    // Handle cry priority - pause BGM when cry starts
    if ((cry || rawCryPointer) && (bgm || rawBgmPointer) && !bgmPaused) {
        if (bgm) bgm->pause();
        if (rawBgmPointer) rawBgmPointer->pause();
        bgmPaused = true;
    }
    
    // Handle BGM transition with fade
    if (pendingBGM && !cry && !rawCryPointer) {
        if (fadeCounter > 0 && fadingOutBGM) {
            fadeCounter--;
            // TODO: Could implement volume fading here
        } else {
            // Complete transition
            if (bgm) {
                bgm->pause();
            }
            if (rawBgmPointer) {
                rawBgmPointer->pause();
                rawBgmPointer = nullptr;
            }
            bgm = std::move(pendingBGM);
            if (bgm) {
                bgm->reset();
                bgm->resume();
            }
            fadingOutBGM = false;
            fadeCounter = 0;
        }
    }
    
    // Clean up finished audio sources
    cleanupFinishedSFX();
    
    // Clean up finished raw SFX pointers
    rawSfxPointers.erase(
        std::remove_if(rawSfxPointers.begin(), rawSfxPointers.end(),
            [](AudioSource* sfx) {
                return !sfx || sfx->isFinished();
            }),
        rawSfxPointers.end()
    );
}

void AudioMixer::setBGM(AudioSource* source) {
    if (bgm) {
        bgm->pause();
    }
    if (rawBgmPointer) {
        rawBgmPointer->pause();
    }
    
    bgm.reset();
    rawBgmPointer = source;
    bgmPaused = false;
    
    if (rawBgmPointer && !cry && !rawCryPointer) {
        rawBgmPointer->reset();
        rawBgmPointer->resume();
    }
}

void AudioMixer::setBGM(std::unique_ptr<AudioSource> source) {
    if (bgm) {
        bgm->pause();
    }
    if (rawBgmPointer) {
        rawBgmPointer->pause();
        rawBgmPointer = nullptr;
    }
    
    bgm = std::move(source);
    bgmPaused = false;
    
    if (bgm && !cry && !rawCryPointer) {
        bgm->reset();
        bgm->resume();
    }
}

void AudioMixer::transitionToBGM(std::unique_ptr<AudioSource> newBGM, bool fadeOut) {
    pendingBGM = std::move(newBGM);
    fadingOutBGM = fadeOut;
    fadeCounter = fadeOut ? 16 : 0; // Fade duration in update ticks
}

void AudioMixer::setCry(AudioSource* source) {
    if (cry) {
        cry->reset();
    }
    if (rawCryPointer) {
        rawCryPointer->reset();
    }
    
    cry.reset();
    rawCryPointer = source;
    
    if (rawCryPointer) {
        rawCryPointer->reset();
        // BGM will be paused in next update() call
    }
}

void AudioMixer::setCry(std::unique_ptr<AudioSource> source) {
    if (cry) {
        cry->reset();
    }
    if (rawCryPointer) {
        rawCryPointer->reset();
        rawCryPointer = nullptr;
    }
    
    cry = std::move(source);
    
    if (cry) {
        cry->reset();
        // BGM will be paused in next update() call
    }
}

void AudioMixer::addSFX(AudioSource* sfx) {
    if (sfx && rawSfxPointers.size() < MAX_SFX_SOURCES) {
        sfx->reset();
        rawSfxPointers.push_back(sfx);
    }
}

void AudioMixer::addSFX(std::unique_ptr<AudioSource> sfx) {
    if (sfx && sfxQueue.size() < MAX_SFX_SOURCES) {
        sfx->reset();
        sfxQueue.push_back(std::move(sfx));
    }
}

void AudioMixer::removeSFX(AudioSource* sfx) {
    rawSfxPointers.erase(
        std::remove(rawSfxPointers.begin(), rawSfxPointers.end(), sfx),
        rawSfxPointers.end()
    );
}

void AudioMixer::stopBGM() {
    if (bgm) {
        bgm->pause();
    }
    if (rawBgmPointer) {
        rawBgmPointer->pause();
        rawBgmPointer = nullptr;
    }
    bgm.reset();
    bgmPaused = false;
}

void AudioMixer::stopCry() {
    cry.reset();
    if (rawCryPointer) {
        rawCryPointer = nullptr;
    }
    
    // Resume BGM if it was paused
    if (bgmPaused && (bgm || rawBgmPointer)) {
        if (bgm) bgm->resume();
        if (rawBgmPointer) rawBgmPointer->resume();
        bgmPaused = false;
    }
}

void AudioMixer::stopAll() {
    stopBGM();
    stopCry();
    sfxQueue.clear();
    rawSfxPointers.clear();
    pendingBGM.reset();
    fadingOutBGM = false;
    fadeCounter = 0;
}

AudioMixer::MixerState AudioMixer::getState() const {
    MixerState state;
    state.hasBGM = (bgm != nullptr);
    state.bgmPaused = bgmPaused;
    state.hasCry = (cry != nullptr);
    state.activeSFXCount = 0;
    
    // Count active SFX
    for (const auto& sfx : sfxQueue) {
        if (sfx && !sfx->isFinished()) {
            state.activeSFXCount++;
        }
    }
    
    state.isFading = fadingOutBGM && fadeCounter > 0;
    return state;
}

void AudioMixer::setMasterVolume(uint8_t volume) {
    masterVolume = volume;
}

uint8_t AudioMixer::getMasterVolume() const {
    return masterVolume;
}

int16_t AudioMixer::softClip(int32_t sample) {
    // Soft clipping to prevent harsh distortion
    if (sample > 32767) {
        return 32767;
    } else if (sample < -32768) {
        return -32768;
    }
    return (int16_t)sample;
}

void AudioMixer::cleanupFinishedSFX() {
    sfxQueue.erase(
        std::remove_if(sfxQueue.begin(), sfxQueue.end(),
            [](const std::unique_ptr<AudioSource>& sfx) {
                return !sfx || sfx->isFinished();
            }),
        sfxQueue.end()
    );
}
