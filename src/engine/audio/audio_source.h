// audio_source.h - Base interface for all audio sources
#pragma once
#include <cstdint>
#include <cstddef>

/**
 * Base interface for all audio sources in the Wisp Engine.
 * Supports BGM, SFX, and procedural cry synthesis.
 */
class AudioSource {
public:
    virtual ~AudioSource() = default;
    
    /**
     * Render audio samples into the provided buffer.
     * @param buffer Output buffer for mono 16-bit samples
     * @param count Number of samples to render
     */
    virtual void render(int16_t* buffer, size_t count) = 0;
    
    /**
     * Check if this audio source has finished playing.
     * @return true if finished, false if still playing
     */
    virtual bool isFinished() const = 0;
    
    /**
     * Reset the audio source to its starting position.
     */
    virtual void reset() = 0;
    
    /**
     * Pause playback (for BGM sources).
     */
    virtual void pause() {}
    
    /**
     * Resume playback (for BGM sources).
     */
    virtual void resume() {}
    
    /**
     * Get the current volume (0-255).
     */
    virtual uint8_t getVolume() const { return 255; }
    
    /**
     * Set the volume (0-255).
     */
    virtual void setVolume(uint8_t volume) {}
};
