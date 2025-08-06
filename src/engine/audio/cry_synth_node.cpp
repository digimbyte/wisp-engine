// cry_synth_node_wcry.cpp - WCRY format procedural synthesizer implementation
#include "cry_synth_node.h"
#include <cmath>
#include <cstring>
#include <algorithm>

#ifndef PI
#define PI 3.14159265359f
#endif

// Default synthesis parameters
static const float BASE_FREQUENCY = 220.0f;  // A3 note
static const float MAX_FREQUENCY = 2093.0f;  // C7 note
static const uint32_t PHASE_SCALE = 0xFFFFFFFF;

CrySynthNode::CrySynthNode(const WCrySequenceData& cryData)
    : sequence(cryData), currentStep(0), stepCounter(0), masterVolume(255) {
    
    // Calculate sample rate from divisor
    sampleRate = 44100 / sequence.header.sampleRateDiv;
    if (sampleRate == 0) sampleRate = 16000; // Fallback
    
    // Calculate samples per automation step
    samplesPerStep = sampleRate / 16; // ~16 steps per second
    
    // Initialize channel states
    memset(channelStates, 0, sizeof(channelStates));
    
    // Set initial parameters from first step
    updateChannelParameters();
}

void CrySynthNode::render(int16_t* buffer, size_t count) {
    for (size_t i = 0; i < count; i++) {
        // Check if we need to advance to next step
        if (stepCounter >= samplesPerStep) {
            currentStep++;
            stepCounter = 0;
            updateChannelParameters();
        }
        
        if (isFinished()) {
            // Fill rest with silence
            memset(&buffer[i], 0, (count - i) * sizeof(int16_t));
            break;
        }
        
        // Mix all 4 channels
        int32_t mixedSample = 0;
        
        for (int ch = 0; ch < 4; ch++) {
            mixedSample += generateChannelSample(ch);
        }
        
        // Apply master volume and clamp
        mixedSample = (mixedSample * masterVolume) >> 10; // Divide by 1024 (4 channels * 256 volume)
        buffer[i] = std::clamp(mixedSample, -32767L, 32767L);
        
        stepCounter++;
    }
}

bool CrySynthNode::isFinished() const {
    return currentStep >= sequence.header.stepCount;
}

void CrySynthNode::reset() {
    currentStep = 0;
    stepCounter = 0;
    memset(channelStates, 0, sizeof(channelStates));
    updateChannelParameters();
}

uint8_t CrySynthNode::getVolume() const {
    return masterVolume;
}

void CrySynthNode::setVolume(uint8_t volume) {
    masterVolume = volume;
}

void CrySynthNode::updateChannelParameters() {
    if (currentStep >= sequence.header.stepCount) return;
    
    for (int ch = 0; ch < 4; ch++) {
        const WCryChannelTrack& track = sequence.channels[ch];
        
        channelStates[ch].currentPitch = parameterToFrequency(track.pitch[currentStep]);
        channelStates[ch].currentSpeed = static_cast<float>(track.speed[currentStep]) / 255.0f;
        channelStates[ch].currentBass = static_cast<float>(track.bass[currentStep]) / 255.0f;
        channelStates[ch].currentVolume = parameterToAmplitude(track.volume[currentStep]);
    }
}

int16_t CrySynthNode::generateChannelSample(int channel) {
    ChannelState& state = channelStates[channel];
    
    if (state.currentVolume <= 0.0f) return 0;
    
    int16_t sample = 0;
    
    switch (static_cast<CryChannelType>(channel)) {
        case CRY_CH_SINE_SLIDE: {
            // Channel 0: Sine wave with pitch slide
            sample = generateSine(state.phase, state.currentVolume);
            
            // Update phase with current pitch
            uint32_t phaseInc = static_cast<uint32_t>((state.currentPitch / sampleRate) * PHASE_SCALE);
            state.phase += phaseInc;
            break;
        }
        
        case CRY_CH_SQUARE_VIBRATO: {
            // Channel 1: Square wave with vibrato
            float vibratoFreq = applyVibrato(state.currentPitch, state.currentSpeed * 10.0f, state.vibratoPhase);
            sample = generateSquare(state.phase, state.currentVolume);
            
            uint32_t phaseInc = static_cast<uint32_t>((vibratoFreq / sampleRate) * PHASE_SCALE);
            state.phase += phaseInc;
            state.vibratoPhase += 0.01f; // Vibrato rate
            break;
        }
        
        case CRY_CH_NOISE_TREMBLE: {
            // Channel 2: White noise with tremble
            float trembleAmp = applyTremble(state.currentVolume, state.currentSpeed, state.tremblePhase);
            sample = generateNoise(trembleAmp);
            
            state.tremblePhase += 0.02f; // Tremble rate
            break;
        }
        
        case CRY_CH_SINE_BASS: {
            // Channel 3: Sine wave with bass swell
            float bassFreq = state.currentPitch * (0.5f + state.currentBass * 0.5f); // Sub-octave to same octave
            sample = generateSine(state.phase, state.currentVolume * state.currentBass);
            
            uint32_t phaseInc = static_cast<uint32_t>((bassFreq / sampleRate) * PHASE_SCALE);
            state.phase += phaseInc;
            break;
        }
    }
    
    return sample;
}

int16_t CrySynthNode::generateSine(uint32_t phase, float amplitude) {
    float normalizedPhase = static_cast<float>(phase) / PHASE_SCALE;
    float sine = sinf(normalizedPhase * 2.0f * PI);
    return static_cast<int16_t>(sine * amplitude * 32767.0f);
}

int16_t CrySynthNode::generateSquare(uint32_t phase, float amplitude) {
    float square = (phase < (PHASE_SCALE / 2)) ? 1.0f : -1.0f;
    return static_cast<int16_t>(square * amplitude * 32767.0f);
}

int16_t CrySynthNode::generateNoise(float amplitude) {
    // Simple linear congruential generator for noise
    static uint32_t noiseSeed = 12345;
    noiseSeed = noiseSeed * 1103515245 + 12345;
    
    float noise = static_cast<float>(static_cast<int32_t>(noiseSeed)) / 2147483648.0f;
    return static_cast<int16_t>(noise * amplitude * 32767.0f);
}

float CrySynthNode::applyVibrato(float baseFreq, float vibratoDepth, float vibratoPhase) {
    float vibrato = sinf(vibratoPhase) * vibratoDepth;
    return baseFreq * (1.0f + vibrato * 0.1f); // 10% max vibrato
}

float CrySynthNode::applyTremble(float baseAmp, float trembleDepth, float tremblePhase) {
    float tremble = sinf(tremblePhase) * trembleDepth;
    return baseAmp * (1.0f + tremble * 0.3f); // 30% max tremble
}

float CrySynthNode::parameterToFrequency(uint8_t param) {
    // Map 0-255 to frequency range (BASE_FREQUENCY to MAX_FREQUENCY)
    float normalized = static_cast<float>(param) / 255.0f;
    return BASE_FREQUENCY + (normalized * (MAX_FREQUENCY - BASE_FREQUENCY));
}

float CrySynthNode::parameterToAmplitude(uint8_t param) {
    return static_cast<float>(param) / 255.0f;
}
