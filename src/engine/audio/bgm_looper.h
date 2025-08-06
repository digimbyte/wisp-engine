// bgm_looper.h - Background music and SFX players for Wisp audio formats
#pragma once
#include "audio_source.h"
#include "audio_formats.h"

/**
 * BGM player with WBGM format support (IMA ADPCM streaming).
 * Handles block-based decompression and seamless looping.
 */
class BGMLooper : public AudioSource {
public:
    /**
     * Create BGM looper from WBGM format data.
     * @param header WBGM format header
     * @param data Compressed ADPCM block data
     */
    BGMLooper(const WBGMHeader* header, const uint8_t* data);
    ~BGMLooper();
    
    // AudioSource interface
    void render(int16_t* buffer, size_t count) override;
    bool isFinished() const override;
    void reset() override;
    void pause() override;
    void resume() override;
    uint8_t getVolume() const override;
    void setVolume(uint8_t volume) override;
    
    /**
     * Set looping behavior.
     */
    void setLooping(bool loop);
    
    /**
     * Get current playback position in samples.
     */
    size_t getPosition() const;

private:
    const WBGMHeader* header;
    const uint8_t* compressedData;
    IMAState decoderState;
    size_t currentBlock;
    size_t sampleInBlock;
    bool paused;
    bool looping;
    uint8_t volume;
    
    // Decode buffer for current block
    int16_t* blockBuffer;
    size_t samplesPerBlock;
    
    void decodeCurrentBlock();
    void advanceToNextBlock();
};

/**
 * SFX player with WSFX format support (IMA ADPCM, higher quality).
 * Optimized for short sound effects with low latency.
 */
class SFXPlayer : public AudioSource {
public:
    /**
     * Create SFX player from WSFX format data.
     * @param header WSFX format header
     * @param data Compressed ADPCM block data
     */
    SFXPlayer(const WSFXHeader* header, const uint8_t* data);
    ~SFXPlayer();
    
    // AudioSource interface
    void render(int16_t* buffer, size_t count) override;
    bool isFinished() const override;
    void reset() override;
    uint8_t getVolume() const override;
    void setVolume(uint8_t volume) override;

private:
    const WSFXHeader* header;
    const uint8_t* compressedData;
    IMAState decoderState;
    size_t currentBlock;
    size_t sampleInBlock;
    uint8_t volume;
    
    // Decode buffer for current block
    int16_t* blockBuffer;
    size_t samplesPerBlock;
    
    void decodeCurrentBlock();
};
