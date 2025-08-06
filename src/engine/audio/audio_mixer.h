// audio_mixer.h - Priority-based audio mixer for BGM, SFX, and Cries
#pragma once
#include "audio_source.h"
#include <vector>
#include <memory>

#define AUDIO_BUFFER_SIZE 256  // Samples per mix tick
#define MAX_SFX_SOURCES 8      // Maximum simultaneous SFX

/**
 * Priority-based audio mixer implementing the Wisp Engine audio model:
 * - BGM: Continuous background music (pausable by cries)
 * - SFX: Sound effects mixed additively with BGM
 * - Cries: Exclusive focus, interrupts BGM until complete
 */
class AudioMixer {
public:
    AudioMixer();
    ~AudioMixer();
    
    /**
     * Initialize the audio mixer system.
     */
    void init();
    
    /**
     * Start the audio mixer processing.
     */
    void start();
    
    /**
     * Shutdown the audio mixer system.
     */
    void shutdown();
    
    /**
     * Mix all active audio sources into output buffer.
     * @param buffer Output buffer for mono 16-bit samples
     * @param samples Number of samples to render
     */
    void mix(int16_t* buffer, size_t samples);
    
    /**
     * Update mixer state (handle priority changes, fades, etc.).
     * Call this once per game frame.
     */
    void update();
    
    /**
     * Set background music source (raw pointer version for API compatibility).
     * @param source BGM audio source (does not take ownership)
     */
    void setBGM(AudioSource* source);
    
    /**
     * Set background music source.
     * @param source BGM audio source (takes ownership)
     */
    void setBGM(std::unique_ptr<AudioSource> source);
    
    /**
     * Transition to new BGM with optional fade.
     * @param newBGM New BGM source (takes ownership)
     * @param fadeOut Whether to fade out current BGM
     */
    void transitionToBGM(std::unique_ptr<AudioSource> newBGM, bool fadeOut = true);
    
    /**
     * Set cry source (interrupts BGM) (raw pointer version for API compatibility).
     * @param source Cry audio source (does not take ownership)
     */
    void setCry(AudioSource* source);
    
    /**
     * Set cry source (interrupts BGM).
     * @param source Cry audio source (takes ownership)
     */
    void setCry(std::unique_ptr<AudioSource> source);
    
    /**
     * Add sound effect to mix queue (raw pointer version for API compatibility).
     * @param sfx SFX audio source (does not take ownership)
     */
    void addSFX(AudioSource* sfx);
    
    /**
     * Add sound effect to mix queue.
     * @param sfx SFX audio source (takes ownership)
     */
    void addSFX(std::unique_ptr<AudioSource> sfx);
    
    /**
     * Remove sound effect from mix queue.
     * @param sfx SFX audio source to remove
     */
    void removeSFX(AudioSource* sfx);
    
    /**
     * Stop background music.
     */
    void stopBGM();
    
    /**
     * Stop current cry (allows BGM to resume).
     */
    void stopCry();
    
    /**
     * Stop all audio sources.
     */
    void stopAll();
    
    /**
     * Set master volume.
     * @param volume Master volume (0-255)
     */
    void setMasterVolume(uint8_t volume);
    
    /**
     * Get master volume.
     * @return Master volume (0-255)
     */
    uint8_t getMasterVolume() const;
    
    /**
     * Get current mixer state for debugging.
     */
    struct MixerState {
        bool hasBGM;
        bool bgmPaused;
        bool hasCry;
        int activeSFXCount;
        bool isFading;
    };
    MixerState getState() const;

private:
    // Audio sources
    std::unique_ptr<AudioSource> bgm;
    std::unique_ptr<AudioSource> pendingBGM;
    std::unique_ptr<AudioSource> cry;
    std::vector<std::unique_ptr<AudioSource>> sfxQueue;
    
    // Raw pointer tracking for API compatibility
    std::vector<AudioSource*> rawSfxPointers;
    AudioSource* rawBgmPointer;
    AudioSource* rawCryPointer;
    
    // State management
    bool bgmPaused;
    bool fadingOutBGM;
    int fadeCounter;
    uint8_t masterVolume;
    bool initialized;
    
    // Temporary buffers for mixing
    int16_t tempBuffer[AUDIO_BUFFER_SIZE];
    
    /**
     * Apply soft clipping to prevent audio overflow.
     * @param sample Input sample
     * @return Clipped sample
     */
    int16_t softClip(int32_t sample);
    
    /**
     * Clean up finished SFX sources.
     */
    void cleanupFinishedSFX();
};
