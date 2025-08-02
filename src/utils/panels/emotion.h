// emotion.h
#pragma once
#include <LovyanGFX.hpp>
#include "../../system/definitions.h"
#include "../../system/settings.h"

namespace Emotion {

static const char* emotionLabels[] = {
  "Happy", "Sad", "Angry", "Excited",
  "Calm", "Anxious", "Confused", "Tired",
  "Playful", "Focused", "Frustrated", "Crying"
};

static const int emotionCount = sizeof(emotionLabels) / sizeof(emotionLabels[0]);
static int selectedIndex = 0;
static bool active = false;
static const char* prompt = nullptr;
static char* outputBuffer = nullptr;
static size_t maxLength = 0;

inline void begin(const char* promptText, char* buffer, size_t len) {
  prompt = promptText;
  outputBuffer = buffer;
  maxLength = len;
  selectedIndex = 0;
  active = true;
}

inline bool isActive() {
  return active;
}

inline void moveLeft() {
  if (--selectedIndex < 0) selectedIndex = emotionCount - 1;
}

inline void moveRight() {
  if (++selectedIndex >= emotionCount) selectedIndex = 0;
}

inline void moveUp() {
  moveLeft();
}

inline void moveDown() {
  moveRight();
}

inline void select() {
  strncpy(outputBuffer, emotionLabels[selectedIndex], maxLength - 1);
  outputBuffer[maxLength - 1] = '\0';
  active = false;
}

inline void render(LGFX& display) {
  if (!active) return;
  display.clear(Settings::theme.background);

  // Prompt
  display.setTextDatum(top_center);
  display.setTextColor(Settings::theme.foreground);
  display.drawString(prompt, SCREEN_WIDTH / 2, 8);

  // Mood grid 4x3
  int cols = 4;
  int rows = 3;
  int boxW = SCREEN_WIDTH / cols;
  int boxH = 30;
  int yStart = 40;

  for (int i = 0; i < emotionCount; ++i) {
    int col = i % cols;
    int row = i / cols;
    int x = col * boxW;
    int y = yStart + row * boxH;

    bool selected = (i == selectedIndex);
    display.fillRect(x + 2, y + 2, boxW - 4, boxH - 4, selected ? Settings::theme.highlight : Settings::theme.secondary);
    display.setTextDatum(top_center);
    display.setTextColor(selected ? Settings::theme.background : Settings::theme.foreground);
    display.drawString(emotionLabels[i], x + boxW / 2, y + 8);
  }
}

} // namespace Emotion
