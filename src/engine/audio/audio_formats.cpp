// audio_formats.cpp - Implementation of Wisp Audio Format Utilities
#include "audio_formats.h"
#include <cstring>
#include <algorithm>

namespace AudioFormats {

// IMA ADPCM step table (standard)
const int16_t IMA_STEP_TABLE[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

// IMA ADPCM index table (standard)
const int8_t IMA_INDEX_TABLE[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

void decodeIMABlock(const uint8_t* compressed, int16_t* output, 
                   IMAState& state, size_t blockSize) {
    
    size_t samplesPerBlock = (blockSize - 4) * 2; // 2 samples per byte
    
    for (size_t i = 0; i < samplesPerBlock; i++) {
        // Get 4-bit ADPCM code
        uint8_t byte = compressed[4 + i/2]; // Skip 4-byte header
        uint8_t code = (i & 1) ? (byte >> 4) : (byte & 0x0F);
        
        // Decode using IMA ADPCM algorithm
        int16_t step = IMA_STEP_TABLE[state.stepIndex];
        int32_t diff = step >> 3;
        
        if (code & 4) diff += step;
        if (code & 2) diff += step >> 1;
        if (code & 1) diff += step >> 2;
        if (code & 8) diff = -diff;
        
        // Update predictor
        int32_t predictor = state.predictor + diff;
        predictor = std::clamp(predictor, -32768L, 32767L);
        state.predictor = static_cast<int16_t>(predictor);
        
        // Update step index
        state.stepIndex += IMA_INDEX_TABLE[code];
        state.stepIndex = std::clamp(static_cast<int32_t>(state.stepIndex), static_cast<int32_t>(0), static_cast<int32_t>(88));
        
        // Output sample
        output[i] = state.predictor;
    }
}

bool validateWBGM(const WBGMHeader* header) {
    if (!header) return false;
    
    // Check magic
    if (memcmp(header->magic, "WBGM", 4) != 0) return false;
    
    // Validate parameters
    if (header->sampleRate < 8000 || header->sampleRate > 16000) return false;
    if (header->channels != 1) return false; // Mono only
    if (header->blockSize < 128 || header->blockSize > 1024) return false;
    if (header->totalBlocks == 0) return false;
    
    return true;
}

bool validateWSFX(const WSFXHeader* header) {
    if (!header) return false;
    
    // Check magic
    if (memcmp(header->magic, "WSFX", 4) != 0) return false;
    
    // Validate parameters
    if (header->sampleRate < 16000 || header->sampleRate > 22050) return false;
    if (header->channels != 1) return false; // Mono only
    if (header->blockSize < 64 || header->blockSize > 512) return false;
    if (header->totalBlocks == 0) return false;
    
    return true;
}

bool validateWCry(const WCryHeader* header) {
    if (!header) return false;
    
    // Check magic
    if (memcmp(header->magic, "WCRY", 4) != 0) return false;
    
    // Validate parameters
    if (header->stepCount == 0 || header->stepCount > 128) return false;
    if (header->sampleRateDiv < 1 || header->sampleRateDiv > 8) return false;
    
    return true;
}

size_t getWBGMDecodedSize(const WBGMHeader* header) {
    if (!header) return 0;
    return header->totalSamples * sizeof(int16_t);
}

size_t getWSFXDecodedSize(const WSFXHeader* header) {
    if (!header) return 0;
    return header->totalSamples * sizeof(int16_t);
}

const char* getFormatName(const void* data) {
    if (!data) return "Unknown";
    
    const char* magic = static_cast<const char*>(data);
    
    if (memcmp(magic, "WBGM", 4) == 0) return "WBGM";
    if (memcmp(magic, "WSFX", 4) == 0) return "WSFX";
    if (memcmp(magic, "WCRY", 4) == 0) return "WCRY";
    
    return "Unknown";
}

} // namespace AudioFormats
