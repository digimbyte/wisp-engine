# Wisp Engine - Micro-Folder Architecture

The Wisp Engine has been reorganized into logical micro-folders for better maintainability and cleaner architecture on ESP32-C6 embedded systems.

## Directory Structure

```
src/engine/
├── engine.h                 # Main engine header (includes all subsystems)
├── app/                     # Application framework and management
│   ├── api_limits.h         # API safety limits and constraints
│   ├── config.h             # App configuration system
│   ├── curated_api.h/.cpp   # High-level curated API for apps
│   ├── interface.h          # App interface definitions
│   ├── loader.h             # App loading system
│   ├── loop.h               # Main app loop definitions
│   ├── loop_manager.h       # Advanced loop management
│   └── native_loader.h      # Native C++ app loader
├── audio/                   # Audio subsystem
│   └── engine.h             # Audio engine and sound processing
├── core/                    # Core engine functionality
│   ├── config.h             # Engine configuration
│   ├── debug.h              # Debug and logging system
│   ├── engine.h             # Core engine management
│   ├── resource_manager.h   # Lazy resource loading
│   └── timing.h             # Timing and frame rate control
├── database/                # Data persistence and storage
│   ├── legacy_system.h/.cpp # Legacy database system
│   ├── partitioned_system.h/.cpp # New partitioned database (16KB LP-SRAM)
│   ├── partitioned_v2.h     # Alternative database implementation
│   └── save_system.h/.cpp   # Save/load system
├── entities/                # Entity-Component system
│   └── system.h             # Entity management and components
├── graphics/                # Graphics and rendering subsystem
│   ├── engine.h             # Main graphics engine
│   ├── framebuffer.h        # Palette framebuffer system
│   ├── lut_system.h/.cpp    # Enhanced LUT (lookup table) system
│   ├── optimized_engine.h   # Optimized graphics engine
│   ├── palette_lut_system.h # Hybrid palette/LUT system
│   ├── palette_system.h     # Palette management
│   ├── particles.h          # Particle system
│   ├── renderer.h           # Low-level rendering
│   ├── sprite_config.h      # Sprite system configuration
│   ├── sprite_layers.h/.cpp # Multi-layer sprite system
│   └── sprite_system.h      # Sprite management
└── physics/                 # Physics and mathematics
    ├── engine.h             # Physics engine and collision detection
    └── math.h               # Vector math and utilities
```

## Usage

### Quick Start
```cpp
#include "engine/engine.h"

void setup() {
    // Initialize all engine subsystems
    if (!WISP_ENGINE_INIT()) {
        Serial.println("Engine initialization failed!");
        return;
    }
    
    // Your app setup code here
}

void loop() {
    // Your app loop
    
    // Graphics example
    auto* gfx = WispEngine::Engine::getGraphics();
    gfx->clear();
    gfx->drawSprite(0, 64, 64);
    gfx->present();
    
    // Database example
    auto* db = WispEngine::Engine::getDatabase();
    db->setU32(WISP_KEY_MAKE(NS_PLAYER, CAT_STATS, 0), playerScore);
}
```

### Subsystem Access
Each subsystem can be accessed individually:

```cpp
// Graphics
#include "engine/graphics/engine.h"
#include "engine/graphics/sprite_system.h"

// Audio
#include "engine/audio/engine.h"

// Database
#include "engine/database/partitioned_system.h"

// Physics
#include "engine/physics/engine.h"
#include "engine/physics/math.h"
```

## Design Principles

1. **Micro-folders**: Each subsystem is contained in its own folder
2. **Clean naming**: No more "wisp_" prefixes cluttering file names
3. **Logical grouping**: Related functionality grouped together
4. **Minimal dependencies**: Each subsystem can be used independently
5. **ESP32-C6 optimized**: All systems designed for 16KB LP-SRAM constraints

## Memory Efficiency

- **Graphics**: Palette-based rendering with LUT optimization
- **Database**: Partitioned system using only required LP-SRAM
- **Physics**: Lightweight collision and math operations
- **Audio**: Efficient PWM and I2S audio processing
- **Entities**: Component-based system with minimal overhead

## Integration

The engine integrates seamlessly with PlatformIO and Arduino IDE for ESP32-C6 development. All systems respect the 16KB LP-SRAM limitation and provide compile-time size validation.

## Compatibility

This organization maintains backward compatibility while providing a cleaner development experience. Old include paths will need to be updated to the new micro-folder structure.
