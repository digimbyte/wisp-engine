// audio_engine.h - Wisp Engine Audio System (Primary Interface)
#pragma once
#include "audio_formats.h"
#include <cstdint>
#include <cstddef>

namespace WispEngine {
namespace Audio {

/**
 * Initialize the audio system.
 * Must be called before any other audio functions.
 */
void init();

/**
 * Shutdown the audio system.
 * Stops all audio and releases resources.
 */
void shutdown();

/**
 * Update audio system.
 * Call this once per frame to handle cleanup and transitions.
 */
void update();

// === BGM (Background Music) Functions ===

/**
 * Play background music from WBGM format data.
 * @param header WBGM format header
 * @param data Compressed ADPCM block data
 * @param volume Volume level (0-255)
 */
void playBGM(const WBGMHeader* header, const uint8_t* data, uint8_t volume = 255);

/**
 * Stop current background music.
 */
void stopBGM();

/**
 * Pause current background music.
 */
void pauseBGM();

/**
 * Resume paused background music.
 */
void resumeBGM();

/**
 * Set BGM volume.
 * @param volume Volume level (0-255)
 */
void setBGMVolume(uint8_t volume);

// === SFX (Sound Effects) Functions ===

/**
 * Play sound effect from WSFX format data.
 * @param header WSFX format header
 * @param data Compressed ADPCM block data
 */
void playSFX(const WSFXHeader* header, const uint8_t* data);

/**
 * Stop all currently playing sound effects.
 */
void stopAllSFX();

// === CRY (Procedural Synthesis) Functions ===

/**
 * Play procedural cry from WCRY format data.
 * @param sequence Complete WCRY sequence data
 */
void playCry(const WCrySequenceData* sequence);

/**
 * Stop current cry playback.
 */
void stopCry();

// === System Control Functions ===

/**
 * Stop all audio (BGM, SFX, and cries).
 */
void stopAll();

/**
 * Check if BGM is currently playing.
 * @return true if BGM is active and not finished
 */
bool isBGMPlaying();

/**
 * Check if a cry is currently playing.
 * @return true if cry is active
 */
bool isCryPlaying();

/**
 * Get number of active sound effects.
 * @return Count of currently playing SFX
 */
size_t getActiveSFXCount();

/**
 * Set master volume for all audio.
 * @param volume Master volume level (0-255)
 */
void setMasterVolume(uint8_t volume);

/**
 * Get current master volume.
 * @return Master volume level (0-255)
 */
uint8_t getMasterVolume();

} // namespace Audio
} // namespace WispEngine
