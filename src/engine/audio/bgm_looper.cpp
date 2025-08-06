// bgm_looper.cpp - Implementation of WBGM and WSFX players
#include "bgm_looper.h"
#include <cstring>
#include <algorithm>

// === BGMLooper Implementation (WBGM Format) ===

BGMLooper::BGMLooper(const WBGMHeader* hdr, const uint8_t* data)
    : header(hdr), compressedData(data), currentBlock(0), sampleInBlock(0),
      paused(false), looping(hdr->looping), volume(255), blockBuffer(nullptr) {
    
    // Calculate samples per block
    samplesPerBlock = (header->blockSize - 4) * 2; // IMA ADPCM: 2 samples per byte
    
    // Allocate decode buffer for one block
    blockBuffer = new int16_t[samplesPerBlock];
    
    // Initialize decoder state from first block
    const WBGMBlock* firstBlock = reinterpret_cast<const WBGMBlock*>(compressedData);
    decoderState.predictor = firstBlock->predictor;
    decoderState.stepIndex = firstBlock->stepIndex;
    
    // Decode first block
    decodeCurrentBlock();
}

BGMLooper::~BGMLooper() {
    delete[] blockBuffer;
}

void BGMLooper::render(int16_t* buffer, size_t count) {
    if (paused || !header || !compressedData) {
        memset(buffer, 0, count * sizeof(int16_t));
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (currentBlock < header->totalBlocks) {
            // Get sample from current block
            int32_t sample = blockBuffer[sampleInBlock];
            sample = (sample * volume) >> 8; // Apply volume
            buffer[i] = std::clamp(sample, -32767L, 32767L);
            
            sampleInBlock++;
            if (sampleInBlock >= samplesPerBlock) {
                advanceToNextBlock();
            }
        } else {
            if (looping) {
                // Loop back to start
                reset();
                if (currentBlock < header->totalBlocks) {
                    int32_t sample = blockBuffer[sampleInBlock];
                    sample = (sample * volume) >> 8;
                    buffer[i] = std::clamp(sample, -32767L, 32767L);
                    sampleInBlock++;
                } else {
                    buffer[i] = 0;
                }
            } else {
                // Fill rest with silence
                memset(&buffer[i], 0, (count - i) * sizeof(int16_t));
                break;
            }
        }
    }
}

bool BGMLooper::isFinished() const {
    return !looping && currentBlock >= header->totalBlocks;
}

void BGMLooper::reset() {
    currentBlock = 0;
    sampleInBlock = 0;
    
    // Reset decoder state
    const WBGMBlock* firstBlock = reinterpret_cast<const WBGMBlock*>(compressedData);
    decoderState.predictor = firstBlock->predictor;
    decoderState.stepIndex = firstBlock->stepIndex;
    
    decodeCurrentBlock();
}

void BGMLooper::pause() {
    paused = true;
}

void BGMLooper::resume() {
    paused = false;
}

uint8_t BGMLooper::getVolume() const {
    return volume;
}

void BGMLooper::setVolume(uint8_t vol) {
    volume = vol;
}

void BGMLooper::setLooping(bool loop) {
    looping = loop;
}

size_t BGMLooper::getPosition() const {
    return currentBlock * samplesPerBlock + sampleInBlock;
}

void BGMLooper::decodeCurrentBlock() {
    if (currentBlock >= header->totalBlocks) return;
    
    const WBGMBlock* block = reinterpret_cast<const WBGMBlock*>(
        compressedData + currentBlock * header->blockSize);
    
    // Update decoder state from block header
    decoderState.predictor = block->predictor;
    decoderState.stepIndex = block->stepIndex;
    
    // Decode the block
    AudioFormats::decodeIMABlock(reinterpret_cast<const uint8_t*>(block),
                                blockBuffer, decoderState, header->blockSize);
}

void BGMLooper::advanceToNextBlock() {
    currentBlock++;
    sampleInBlock = 0;
    
    if (currentBlock < header->totalBlocks) {
        decodeCurrentBlock();
    }
}

// === SFXPlayer Implementation (WSFX Format) ===

SFXPlayer::SFXPlayer(const WSFXHeader* hdr, const uint8_t* data)
    : header(hdr), compressedData(data), currentBlock(0), sampleInBlock(0),
      volume(hdr->volume), blockBuffer(nullptr) {
    
    // Calculate samples per block
    samplesPerBlock = (header->blockSize - 4) * 2; // IMA ADPCM: 2 samples per byte
    
    // Allocate decode buffer for one block
    blockBuffer = new int16_t[samplesPerBlock];
    
    // Initialize decoder state from first block
    const WSFXBlock* firstBlock = reinterpret_cast<const WSFXBlock*>(compressedData);
    decoderState.predictor = firstBlock->predictor;
    decoderState.stepIndex = firstBlock->stepIndex;
    
    // Decode first block
    decodeCurrentBlock();
}

SFXPlayer::~SFXPlayer() {
    delete[] blockBuffer;
}

void SFXPlayer::render(int16_t* buffer, size_t count) {
    if (!header || !compressedData) {
        memset(buffer, 0, count * sizeof(int16_t));
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (currentBlock < header->totalBlocks) {
            // Get sample from current block
            int32_t sample = blockBuffer[sampleInBlock];
            sample = (sample * volume) >> 8; // Apply volume
            buffer[i] = std::clamp(sample, -32767L, 32767L);
            
            sampleInBlock++;
            if (sampleInBlock >= samplesPerBlock) {
                currentBlock++;
                sampleInBlock = 0;
                if (currentBlock < header->totalBlocks) {
                    decodeCurrentBlock();
                }
            }
        } else {
            // Fill rest with silence
            memset(&buffer[i], 0, (count - i) * sizeof(int16_t));
            break;
        }
    }
}

bool SFXPlayer::isFinished() const {
    return currentBlock >= header->totalBlocks;
}

void SFXPlayer::reset() {
    currentBlock = 0;
    sampleInBlock = 0;
    
    // Reset decoder state
    const WSFXBlock* firstBlock = reinterpret_cast<const WSFXBlock*>(compressedData);
    decoderState.predictor = firstBlock->predictor;
    decoderState.stepIndex = firstBlock->stepIndex;
    
    decodeCurrentBlock();
}

uint8_t SFXPlayer::getVolume() const {
    return volume;
}

void SFXPlayer::setVolume(uint8_t vol) {
    volume = vol;
}

void SFXPlayer::decodeCurrentBlock() {
    if (currentBlock >= header->totalBlocks) return;
    
    const WSFXBlock* block = reinterpret_cast<const WSFXBlock*>(
        compressedData + currentBlock * header->blockSize);
    
    // Update decoder state from block header
    decoderState.predictor = block->predictor;
    decoderState.stepIndex = block->stepIndex;
    
    // Decode the block
    AudioFormats::decodeIMABlock(reinterpret_cast<const uint8_t*>(block),
                                blockBuffer, decoderState, header->blockSize);
}
