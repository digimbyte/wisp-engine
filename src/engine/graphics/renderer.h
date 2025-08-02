// palette_renderer.h
#pragma once
#include <stdint.h>
#include <string.h>

#define MAX_PALETTES 4
#define PALETTE_SIZE 256

// RGB565 color output
using Color = uint16_t;

// A palette entry can be static or animated
struct PaletteEntry {
  Color baseColor;
  const Color* animationFrames;  // Optional pointer to animation sequence
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

  void loadPalette(uint8_t slot, const Color* colors) {
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

  void setAnimation(uint8_t slot, uint8_t index, const Color* frames, uint8_t count, uint16_t durationMs) {
    if (slot >= MAX_PALETTES || index >= PALETTE_SIZE) return;
    palettes[slot][index].animationFrames = frames;
    palettes[slot][index].frameCount = count;
    palettes[slot][index].frameDurationMs = durationMs;
    palettes[slot][index].lastUpdate = millis();
    palettes[slot][index].currentFrame = 0;
  }

  void updateAnimations() {
    uint32_t now = millis();
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

  Color resolveColor(uint8_t paletteId, uint8_t index) const {
    if (paletteId >= MAX_PALETTES || index >= PALETTE_SIZE) return 0;
    const PaletteEntry& entry = palettes[paletteId][index];
    if (entry.animationFrames && entry.frameCount > 0) {
      return entry.animationFrames[entry.currentFrame];
    } else {
      return entry.baseColor;
    }
  }
};
