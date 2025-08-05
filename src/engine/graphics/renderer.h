// palette_renderer.h
#pragma once
#include <stdint.h>
#include <string.h>
#include "../../system/esp32_common.h"

#define MAX_PALETTES 4
#define PALETTE_SIZE 256

// RGB565 Color565 output
using Color565 = uint16_t;

// A palette entry can be static or animated
struct PaletteEntry {
  Color565 baseColor;
  const Color565* animationFrames;  // Optional pointer to animation sequence
  uint8_t frameCount;
  uint8_t currentFrame;
  uint16_t frameDurationMs;
  uint32_t lastUpdate;
};

class ColorRenderer {
public:
  PaletteEntry palettes[MAX_PALETTES][PALETTE_SIZE];

  void init() {
    memset(palettes, 0, sizeof(palettes));
  }

  void loadPalette(uint8_t slot, const Color565* colors) {
    if (slot >= MAX_PALETTES) return;
    for (int i = 0; i < PALETTE_SIZE; ++i) {
      palettes[slot][i].baseColor = colors[i];
      palettes[slot][i].animationFrames = nullptr;
      palettes[slot][i].frameCount = 0;
      palettes[slot][i].currentFrame = 0;
      palettes[slot][i].frameDurationMs = 0;
      palettes[slot][i].lastUpdate = 0;
    }
  }

  void setAnimation(uint8_t slot, uint8_t index, const Color565* frames, uint8_t count, uint16_t durationMs) {
    if (slot >= MAX_PALETTES) return;  // index is uint8_t (0-255), PALETTE_SIZE is 256, so index is always valid
    palettes[slot][index].animationFrames = frames;
    palettes[slot][index].frameCount = count;
    palettes[slot][index].frameDurationMs = durationMs;
    palettes[slot][index].lastUpdate = get_millis();
    palettes[slot][index].currentFrame = 0;
  }

  void updateAnimations() {
    uint32_t now = get_millis();
    for (uint8_t s = 0; s < MAX_PALETTES; ++s) {
      for (uint16_t i = 0; i < PALETTE_SIZE; ++i) {
        auto& entry = palettes[s][i];
        if (entry.animationFrames && entry.frameCount > 0) {
          if (now - entry.lastUpdate >= entry.frameDurationMs) {
            entry.currentFrame = (entry.currentFrame + 1) % entry.frameCount;
            entry.lastUpdate = now;
          }
        }
      }
    }
  }

  Color565 resolveColor(uint8_t paletteId, uint8_t index) const {
    if (paletteId >= MAX_PALETTES) return 0;  // index is uint8_t (0-255), PALETTE_SIZE is 256, so index is always valid
    const PaletteEntry& entry = palettes[paletteId][index];
    if (entry.animationFrames && entry.frameCount > 0) {
      return entry.animationFrames[entry.currentFrame];
    } else {
      return entry.baseColor;
    }
  }
};
