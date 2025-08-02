// boot_ui.h
#pragma once
#include <LovyanGFX.hpp>
#include "definitions.h"
#include "boot_state.h"

namespace BootUI {

static bool fadeTriggered = false;
static uint32_t fadeStart = 0;
static const uint32_t fadeDuration = 200; // ms

inline void triggerFadeOut() {
  fadeTriggered = true;
  fadeStart = millis();
}

inline bool isFadeDone() {
  if (!fadeTriggered) return false;
  return millis() - fadeStart >= fadeDuration;
}

inline void renderSplash(LGFX &display) {
  display.fillScreen(TFT_BLACK);

  // Render logo (placeholder box, replace with actual bitmap loading if available)
  display.fillRect(40, 20, 240, 80, TFT_DARKGREY);

  // Render "Wisp Engine" text
  display.setTextColor(TFT_WHITE);
  display.setTextDatum(top_center);
  display.drawString("Wisp Engine", SCREEN_WIDTH / 2, 110);

#ifdef DEBUG_ENABLED
  // Render current boot phase
  display.setTextColor(TFT_YELLOW);
  display.setTextDatum(bottom_center);
  display.drawString(BootState::getPhaseName(), SCREEN_WIDTH / 2, SCREEN_HEIGHT - 12);
#endif

  // Handle fade out
  if (fadeTriggered) {
    uint32_t elapsed = millis() - fadeStart;
    if (elapsed < fadeDuration) {
      uint8_t fade = map(elapsed, 0, fadeDuration, 0, 255);
      uint16_t overlay = display.color888(fade, fade, fade); // light gray fade mask
      display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, overlay);
    } else {
      fadeTriggered = false; // Reset once done
    }
  }
}

} // namespace BootUI
