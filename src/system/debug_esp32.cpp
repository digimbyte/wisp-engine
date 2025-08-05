// debug_esp32.cpp - Static variable definitions for DebugSystem
#include "debug_esp32.h"

// Static member definitions
bool DebugSystem::debugMode = false;
bool DebugSystem::safetyDisabled = false;
uint32_t DebugSystem::errorCount = 0;
uint32_t DebugSystem::warningCount = 0;
uint32_t DebugSystem::lastHeartbeat = 0;
bool DebugSystem::pinsInitialized = false;
std::string DebugSystem::currentAppName = "";
uint32_t DebugSystem::errorsThisSecond = 0;
uint32_t DebugSystem::lastErrorSecond = 0;
const char* DebugSystem::TAG = "DEBUG";
