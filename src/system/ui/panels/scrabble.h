// scrabble.h
#pragma once
#include <LovyanGFX.hpp>
#include "definitions.h"
#include "system/settings.h"

namespace Scrabble {

static char* targetBuffer = nullptr;
static size_t maxLength = 0;
static const char* prompt = nullptr;
static int cursorPos = 0;
static int columnIndex = 0;
static int offset = 0;

static bool active = false;

static const int columnCount = 7;
static const int rowCount = 6;

static const char keyMap[rowCount][columnCount] = {
  {'A','B','C','D','E','F','G'},
  {'H','I','J','K','L','M','N'},
  {'O','P','Q','R','S','T','U'},
  {'V','W','X','Y','Z','0','1'},
  {'2','3','4','5','6','7','8'},
  {'9','-','_',' ',' ',' ',' '}
};

inline void begin(const char* promptText, char* buffer, size_t len) {
  prompt = promptText;
  targetBuffer = buffer;
  maxLength = len;
  cursorPos = strlen(buffer);
  columnIndex = 0;
  offset = 0;
  active = true;
}

inline bool isActive() {
  return active;
}

inline void moveLeft() {
  if (--columnIndex < 0) columnIndex = columnCount - 1;
}

inline void moveRight() {
  if (++columnIndex >= columnCount) columnIndex = 0;
}

inline void moveUp() {
  if (--offset < 0) offset = rowCount - 1;
}

inline void moveDown() {
  if (++offset >= rowCount) offset = 0;
}

inline void selectChar() {
  if (!active || cursorPos >= maxLength - 1) return;
  char selected = keyMap[offset][columnIndex];
  targetBuffer[cursorPos++] = selected;
  targetBuffer[cursorPos] = '\0';
}

inline void backspace() {
  if (!active || cursorPos <= 0) return;
  targetBuffer[--cursorPos] = '\0';
}

inline void accept() {
  active = false;
  while (cursorPos > 0 && targetBuffer[cursorPos - 1] == ' ') {
    targetBuffer[--cursorPos] = '\0';
  }
}

inline void render(LGFX& display) {
  if (!active) return;

  display.clear(Settings::theme.background);

  // Prompt
  display.setTextDatum(top_center);
  display.setTextColor(Settings::theme.foreground);
  display.drawString(prompt, SCREEN_WIDTH / 2, 8);

  // Progress bar
  int barWidth = (SCREEN_WIDTH * cursorPos) / maxLength;
  display.fillRect(0, 24, barWidth, 4, Settings::theme.highlight);
  display.drawRect(0, 24, SCREEN_WIDTH, 4, Settings::theme.secondary);

  // Text buffer preview
  display.setTextDatum(top_left);
  display.setTextColor(Settings::theme.primary);
  display.drawString(targetBuffer, 10, 34);

  // Character row
  display.setTextDatum(top_center);
  int spacing = SCREEN_WIDTH / (columnCount + 1);
  for (int i = 0; i < columnCount; ++i) {
    int x = spacing * (i + 1);
    int y = 70;
    char ch = keyMap[offset][i];
    char str[2] = {ch, '\0'};

    if (i == columnIndex) {
      display.fillRect(x - 8, y - 2, 16, 18, Settings::theme.highlight);
      display.setTextColor(Settings::theme.background);
    } else {
      display.setTextColor(Settings::theme.foreground);
    }
    display.drawString(str, x, y);
  }

  // Optional visual icon slot
  display.drawRect(2, SCREEN_HEIGHT - 18, 16, 16, Settings::theme.secondary);
}

} // namespace Scrabble
