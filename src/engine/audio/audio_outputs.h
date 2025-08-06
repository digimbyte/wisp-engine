// audio_outputs.h - Audio output type definitions for Wisp Engine
#pragma once

// Audio output type bitmasks
#define AUDIO_PIEZO         0x01    // Built-in piezo speaker
#define AUDIO_I2S_DAC       0x02    // I2S DAC output  
#define AUDIO_BLUETOOTH     0x04    // Bluetooth audio output
#define AUDIO_PWM           0x08    // PWM audio output
#define AUDIO_INTERNAL_DAC  0x10    // Internal DAC output
#define AUDIO_ALL           0xFF    // All available outputs

// Audio output capabilities based on hardware
#ifdef AUDIO_HAS_PIEZO
    #define AUDIO_CAPABILITY_PIEZO AUDIO_PIEZO
#else
    #define AUDIO_CAPABILITY_PIEZO 0
#endif

#ifdef AUDIO_HAS_I2S
    #define AUDIO_CAPABILITY_I2S AUDIO_I2S_DAC
#else
    #define AUDIO_CAPABILITY_I2S 0
#endif

#ifdef AUDIO_HAS_BLUETOOTH
    #define AUDIO_CAPABILITY_BLUETOOTH AUDIO_BLUETOOTH
#else
    #define AUDIO_CAPABILITY_BLUETOOTH 0
#endif

// Combined hardware capabilities
#define AUDIO_HARDWARE_CAPABILITIES (AUDIO_CAPABILITY_PIEZO | AUDIO_CAPABILITY_I2S | AUDIO_CAPABILITY_BLUETOOTH)
