// theme_menu.h
#pragma once
#include <LovyanGFX.hpp>
#include "definitions.h"
#include "system/settings.h"

namespace ThemeMenu {

static int selectedSwatch = 0;
static const char* labelNames[] = {
  "Primary", "Secondary", "Highlight", "Background", "Foreground"
};
static const int labelCount = sizeof(labelNames) / sizeof(labelNames[0]);

// Editable theme name
static char themeName[16] = "Custom";

inline void update() {
  // TODO: Respond to input, allow swatch selection and cycling color presets
  selectedSwatch = (selectedSwatch + 1) % labelCount;
}

inline void render(LGFX& display) {
  display.clear(Settings::theme.background);

  display.setTextDatum(top_center);
  display.setTextColor(Settings::theme.foreground);
  display.drawString("Edit Theme", SCREEN_WIDTH / 2, 10);

  // Theme name field
  display.drawString(themeName, SCREEN_WIDTH / 2, 30);

  // Render swatches
  for (int i = 0; i < labelCount; ++i) {
    int y = 60 + i * 20;
    uint16_t color = 0;
    switch (i) {
      case 0: color = Settings::theme.primary; break;
      case 1: color = Settings::theme.secondary; break;
      case 2: color = Settings::theme.highlight; break;
      case 3: color = Settings::theme.background; break;
      case 4: color = Settings::theme.foreground; break;
    }

    if (i == selectedSwatch) {
      display.fillRect(20, y, SCREEN_WIDTH - 40, 18, Settings::theme.highlight);
      display.setTextColor(Settings::theme.background);
    } else {
      display.fillRect(20, y, SCREEN_WIDTH - 40, 18, color);
      display.setTextColor(Settings::theme.foreground);
    }
    display.drawString(labelNames[i], SCREEN_WIDTH / 2, y + 2);
  }
}

inline void updateAndRender(LGFX& display) {
  update();
  render(display);
}

} // namespace ThemeMenu
