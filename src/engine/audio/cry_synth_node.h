// cry_synth_node.h - Procedural cry synthesizer using standardized WCRY format
#pragma once
#include "audio_source.h"
#include "audio_formats.h"

/**
 * Fixed synthesizer channel types (hardcoded behavior)
 */
enum CryChannelType {
    CRY_CH_SINE_SLIDE = 0,    // Sine wave with pitch slide (main tone)
    CRY_CH_SQUARE_VIBRATO = 1, // Square wave with vibrato (robotic pulse)
    CRY_CH_NOISE_TREMBLE = 2,  // White noise with tremble/burst (texture)
    CRY_CH_SINE_BASS = 3       // Sine wave with bass swell (reinforcement)
};

/**
 * Real-time procedural cry synthesizer.
 * Renders WCRY format control data into audio samples using fixed channel behaviors.
 */
class CrySynthNode : public AudioSource {
public:
    /**
     * Create a cry synthesizer from WCRY format data.
     * @param cryData Complete WCRY sequence data structure
     */
    explicit CrySynthNode(const WCrySequenceData& cryData);
    
    // AudioSource interface
    void render(int16_t* buffer, size_t count) override;
    bool isFinished() const override;
    void reset() override;
    uint8_t getVolume() const override;
    void setVolume(uint8_t volume) override;

private:
    WCrySequenceData sequence;
    uint32_t currentStep;
    uint32_t stepCounter;
    uint32_t samplesPerStep;
    uint32_t sampleRate;
    uint8_t masterVolume;
    
    // Channel state for synthesis
    struct ChannelState {
        uint32_t phase;           // Current phase for oscillator
        float currentPitch;       // Interpolated pitch value
        float currentSpeed;       // Interpolated speed value
        float currentBass;        // Interpolated bass value
        float currentVolume;      // Interpolated volume value
        float vibratoPhase;      // For vibrato effects
        float tremblePhase;      // For tremble effects
    } channelStates[4];
    
    /**
     * Update channel parameters for current step with interpolation.
     */
    void updateChannelParameters();
    
    /**
     * Generate a single sample for a specific channel.
     * @param channel Channel index (0-3)
     * @return Generated sample (-32767 to 32767)
     */
    int16_t generateChannelSample(int channel);
    
    /**
     * Generate sine wave sample.
     */
    int16_t generateSine(uint32_t phase, float amplitude);
    
    /**
     * Generate square wave sample.
     */
    int16_t generateSquare(uint32_t phase, float amplitude);
    
    /**
     * Generate white noise sample.
     */
    int16_t generateNoise(float amplitude);
    
    /**
     * Apply vibrato effect to frequency.
     */
    float applyVibrato(float baseFreq, float vibratoDepth, float vibratoPhase);
    
    /**
     * Apply tremble effect to amplitude.
     */
    float applyTremble(float baseAmp, float trembleDepth, float tremblePhase);
    
    /**
     * Convert 0-255 parameter to frequency (Hz).
     */
    float parameterToFrequency(uint8_t param);
    
    /**
     * Convert 0-255 parameter to amplitude (0.0-1.0).
     */
    float parameterToAmplitude(uint8_t param);
};
