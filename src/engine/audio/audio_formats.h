// audio_formats.h - Wisp Engine Audio Format Definitions
#pragma once
#include <cstdint>
#include <cstddef>

// ===== WBGM FORMAT (Background Music - IMA ADPCM) =====

struct WBGMHeader {
    char magic[4];           // "WBGM"
    uint32_t version;        // Format version
    uint32_t sampleRate;     // 8-16kHz typically
    uint32_t channels;       // 1 (mono)
    uint32_t blockSize;      // 256 bytes typical (sector-aligned)
    uint32_t totalBlocks;    // Number of ADPCM blocks
    uint32_t totalSamples;   // Uncompressed sample count
    uint8_t looping;         // 1 = loop, 0 = one-shot
    uint8_t reserved[3];     // Padding
};

struct WBGMBlock {
    int16_t predictor;       // IMA ADPCM predictor
    uint8_t stepIndex;       // IMA ADPCM step index  
    uint8_t reserved;        // Padding
    uint8_t data[252];       // Compressed ADPCM data (blockSize - 4)
};

// ===== WSFX FORMAT (Sound Effects - IMA ADPCM) =====

struct WSFXHeader {
    char magic[4];           // "WSFX"
    uint32_t version;        // Format version
    uint32_t sampleRate;     // 16-22kHz typically (higher quality)
    uint32_t channels;       // 1 (mono)
    uint32_t blockSize;      // Smaller blocks for low latency
    uint32_t totalBlocks;    // Number of ADPCM blocks
    uint32_t totalSamples;   // Uncompressed sample count
    uint8_t volume;          // Default volume (0-255)
    uint8_t reserved[3];     // Padding
};

struct WSFXBlock {
    int16_t predictor;       // IMA ADPCM predictor
    uint8_t stepIndex;       // IMA ADPCM step index
    uint8_t reserved;        // Padding
    uint8_t data[];          // Variable compressed ADPCM data
};

// ===== WCRY FORMAT (Procedural Cries - MIDI-like) =====

struct WCryChannelTrack {
    uint8_t pitch[64];       // Pitch automation (0-255)
    uint8_t speed[64];       // Speed/rate automation (0-255)
    uint8_t bass[64];        // Bass/filter automation (0-255)
    uint8_t volume[64];      // Volume automation (0-255)
};

struct WCryHeader {
    char magic[4];           // "WCRY"
    uint32_t version;        // Format version
    uint8_t stepCount;       // Number of automation steps (typically 64)
    uint8_t sampleRateDiv;   // Sample rate divisor (2=22kHz, 3=16kHz, etc)
    uint8_t fadeInSteps;     // Auto fade-in duration
    uint8_t fadeOutSteps;    // Auto fade-out duration
    uint32_t reserved;       // Future use
};

struct WCrySequenceData {
    WCryHeader header;
    WCryChannelTrack channels[4];  // Fixed 4-channel synthesizer
};

// ===== IMA ADPCM DECODER STATE =====

struct IMAState {
    int16_t predictor;       // Current predictor value
    uint8_t stepIndex;       // Current step index (0-88)
    uint8_t reserved;        // Padding
};

// ===== AUDIO FORMAT UTILITIES =====

namespace AudioFormats {
    
    // IMA ADPCM step table (shared by WBGM and WSFX)
    extern const int16_t IMA_STEP_TABLE[89];
    extern const int8_t IMA_INDEX_TABLE[16];
    
    // Decode single IMA ADPCM block
    void decodeIMABlock(const uint8_t* compressed, int16_t* output, 
                       IMAState& state, size_t blockSize);
    
    // Validate format headers
    bool validateWBGM(const WBGMHeader* header);
    bool validateWSFX(const WSFXHeader* header);
    bool validateWCry(const WCryHeader* header);
    
    // Calculate decompressed size
    size_t getWBGMDecodedSize(const WBGMHeader* header);
    size_t getWSFXDecodedSize(const WSFXHeader* header);
    
    // Get format info
    const char* getFormatName(const void* data);
    
} // namespace AudioFormats
