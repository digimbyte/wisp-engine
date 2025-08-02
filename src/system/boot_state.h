// boot_state.h
#pragma once

namespace BootState {

enum BootPhase {
  BOOT_START = 0,
  DISPLAY_READY,
  INPUT_READY,
  EVENT_BRIDGE_READY,
  PALETTE_READY,
  PARTICLES_READY,
  AUDIO_READY,
  CPP_ENGINE_READY,
  APPS_SCANNED,
  BOOT_COMPLETE
};

static BootPhase currentPhase = BOOT_START;

inline void advance() {
  if (currentPhase < BOOT_COMPLETE) {
    currentPhase = static_cast<BootPhase>(currentPhase + 1);
  }
}

inline BootPhase getPhase() {
  return currentPhase;
}

inline bool isReady() {
  return currentPhase == BOOT_COMPLETE;
}

inline const char* getPhaseName() {
  switch (currentPhase) {
    case BOOT_START: return "Start";
    case DISPLAY_READY: return "Display";
    case INPUT_READY: return "Input";
    case EVENT_BRIDGE_READY: return "EventBridge";
    case PALETTE_READY: return "Palette";
    case PARTICLES_READY: return "Particles";
    case AUDIO_READY: return "Audio";
    case CPP_ENGINE_READY: return "C++ Engine";
    case APPS_SCANNED: return "Apps";
    case BOOT_COMPLETE: return "Complete";
    default: return "Unknown";
  }
}

} // namespace BootState
