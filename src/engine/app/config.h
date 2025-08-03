// engine/app/config.h - Compile-time Configuration for Wisp Apps
#ifndef WISP_APP_CONFIG_H
#define WISP_APP_CONFIG_H

// This header should be included by apps to configure debug and safety settings
// Apps can override these settings by defining them before including this header

// =============================================================================
// DEBUG AND SAFETY CONFIGURATION
// =============================================================================

// Override these in your app's main file BEFORE including wisp headers:
// #define WISP_APP_DEBUG_MODE true
// #define WISP_APP_SAFETY_DISABLED true
// #include "wisp_engine.h"

#ifndef WISP_APP_DEBUG_MODE
    #define WISP_APP_DEBUG_MODE false       // Default: debug disabled
#endif

#ifndef WISP_APP_SAFETY_DISABLED  
    #define WISP_APP_SAFETY_DISABLED false  // Default: safety enabled
#endif

#ifndef WISP_APP_LOG_TO_SD
    #define WISP_APP_LOG_TO_SD true         // Default: log to SD card
#endif

#ifndef WISP_APP_DEBUG_PINS
    #define WISP_APP_DEBUG_PINS true        // Default: use debug pins
#endif

// =============================================================================
// DEVELOPER CONVENIENCE MACROS
// =============================================================================

// Quick configuration presets for common scenarios

// DEVELOPMENT MODE: Enable all debugging, keep safety
#ifdef WISP_DEV_MODE
    #undef WISP_APP_DEBUG_MODE
    #undef WISP_APP_SAFETY_DISABLED
    #undef WISP_APP_LOG_TO_SD
    #undef WISP_APP_DEBUG_PINS
    
    #define WISP_APP_DEBUG_MODE true
    #define WISP_APP_SAFETY_DISABLED false
    #define WISP_APP_LOG_TO_SD true
    #define WISP_APP_DEBUG_PINS true
#endif

// STRESS TEST MODE: Disable safety, enable debugging
#ifdef WISP_STRESS_TEST_MODE
    #undef WISP_APP_DEBUG_MODE
    #undef WISP_APP_SAFETY_DISABLED
    #undef WISP_APP_LOG_TO_SD
    #undef WISP_APP_DEBUG_PINS
    
    #define WISP_APP_DEBUG_MODE true
    #define WISP_APP_SAFETY_DISABLED true
    #define WISP_APP_LOG_TO_SD true
    #define WISP_APP_DEBUG_PINS true
#endif

// PRODUCTION MODE: Disable debugging, enable safety
#ifdef WISP_PRODUCTION_MODE
    #undef WISP_APP_DEBUG_MODE
    #undef WISP_APP_SAFETY_DISABLED
    #undef WISP_APP_LOG_TO_SD
    #undef WISP_APP_DEBUG_PINS
    
    #define WISP_APP_DEBUG_MODE false
    #define WISP_APP_SAFETY_DISABLED false
    #define WISP_APP_LOG_TO_SD false
    #define WISP_APP_DEBUG_PINS false
#endif

// =============================================================================
// COMPILE-TIME VALIDATION
// =============================================================================

// Warning messages for dangerous configurations
#if WISP_APP_SAFETY_DISABLED && !WISP_APP_DEBUG_MODE
    #warning "DANGER: Safety disabled without debug mode - system may crash silently!"
#endif

#if WISP_APP_SAFETY_DISABLED
    #pragma message "WARNING: Safety limits disabled - use only for stress testing!"
#endif

// =============================================================================
// CONFIGURATION SUMMARY
// =============================================================================

// These will be printed during app initialization
#define WISP_CONFIG_SUMMARY() do { \
    Serial.println("=== Wisp App Configuration ==="); \
    Serial.print("Debug Mode: "); \
    Serial.println(WISP_APP_DEBUG_MODE ? "ENABLED" : "DISABLED"); \
    Serial.print("Safety Limits: "); \
    Serial.println(WISP_APP_SAFETY_DISABLED ? "DISABLED" : "ENABLED"); \
    Serial.print("SD Logging: "); \
    Serial.println(WISP_APP_LOG_TO_SD ? "ENABLED" : "DISABLED"); \
    Serial.print("Debug Pins: "); \
    Serial.println(WISP_APP_DEBUG_PINS ? "ENABLED" : "DISABLED"); \
    Serial.println("============================="); \
} while(0)

// =============================================================================
// APP-SPECIFIC QUOTA OVERRIDES
// =============================================================================

// Apps can override default quotas for their specific needs
// Example usage in app:
// #define WISP_APP_MAX_ENTITIES 128
// #include "app/config.h"

#ifdef WISP_APP_MAX_ENTITIES
    #undef WISP_MAX_ENTITIES
    #define WISP_MAX_ENTITIES WISP_APP_MAX_ENTITIES
#endif

#ifdef WISP_APP_MAX_SPRITES
    #undef WISP_MAX_SPRITES
    #define WISP_MAX_SPRITES WISP_APP_MAX_SPRITES
#endif

#ifdef WISP_APP_MAX_MEMORY
    #undef WISP_MAX_APP_MEMORY
    #define WISP_MAX_APP_MEMORY WISP_APP_MAX_MEMORY
#endif

#ifdef WISP_APP_MAX_AUDIO_CHANNELS
    #undef WISP_MAX_AUDIO_CHANNELS
    #define WISP_MAX_AUDIO_CHANNELS WISP_APP_MAX_AUDIO_CHANNELS
#endif

#ifdef WISP_APP_MAX_PARTICLES
    #undef WISP_MAX_PARTICLES
    #define WISP_MAX_PARTICLES WISP_APP_MAX_PARTICLES
#endif

// =============================================================================
// EXAMPLE USAGE IN APPS
// =============================================================================

/*
EXAMPLE 1: Production app with standard safety
--------------------------------------------
#include "app/config.h"  // Uses defaults: debug=false, safety=enabled
#include "wisp_engine.h"

EXAMPLE 2: Development with debugging
-----------------------------------
#define WISP_DEV_MODE
#include "app/config.h"
#include "wisp_engine.h"

EXAMPLE 3: Stress testing mode
-----------------------------
#define WISP_STRESS_TEST_MODE
#include "app/config.h"
#include "wisp_engine.h"

EXAMPLE 4: Custom configuration
------------------------------
#define WISP_APP_DEBUG_MODE true
#define WISP_APP_SAFETY_DISABLED false
#define WISP_APP_MAX_ENTITIES 128
#include "app/config.h"
#include "wisp_engine.h"

EXAMPLE 5: Performance-critical app
---------------------------------
#define WISP_APP_DEBUG_MODE false
#define WISP_APP_SAFETY_DISABLED false
#define WISP_APP_LOG_TO_SD false
#define WISP_APP_DEBUG_PINS false
#define WISP_APP_MAX_DRAW_CALLS 512
#include "app/config.h"
#include "wisp_engine.h"
*/

#endif // WISP_APP_CONFIG_H
