# Enhanced LUT/Palette System Documentation

## Overview

The Enhanced LUT (Look-Up Table) system replaces the previous 32KB LUT with an efficient 64×64 palette system that includes 4 dynamic transparent slots for real-time color animation effects.

## Key Features

- **Memory Efficient**: 8KB (64×64) vs previous 32KB (128×128)
- **Dynamic Transparency**: 4 slots that can animate through color sequences
- **Binary Transparency**: RGB565 or null (0x0000) - no RGBA blending
- **Zero CPU Overhead**: Animations tied to app frame tick counter
- **Backward Compatible**: Works with existing sprite system

## Architecture

### Base LUT Structure
- **Dimensions**: 64×64 pixels (4096 colors)
- **Format**: RGB565 (16-bit per color)
- **Size**: 8192 bytes total
- **Source**: Uses existing `lut_palette_data.h` (user's converted PNG)

### Dynamic Transparent Slots
- **Location**: Row 64, Columns 61-64 (0-indexed: row 63, cols 60-63)
- **Count**: 4 slots total
- **Default State**: Transparent (0x0000 = completely culled)
- **Animation**: Each slot cycles through user-defined color sequence per app frame

### Slot Mapping
```
LUT Position    | Slot Index | Array Index
(63, 60)        | Slot 3     | 4092
(63, 61)        | Slot 0     | 4093  
(63, 62)        | Slot 1     | 4094
(63, 63)        | Slot 2     | 4095
```

## Usage

### Basic Setup
```cpp
#include "enhanced_lut_system.h"
#include "lut_palette_data.h"

// Load base LUT data
enhancedLUT.loadBaseLUT(lut_palette_lut, LUT_PALETTE_LUT_SIZE);

// Configure slot animations
uint16_t fireColors[] = {0xF800, 0xF940, 0xFB60, 0xFFE0}; // Red to yellow
enhancedLUT.setSlotSequence(0, fireColors, 4);

// Each app frame, update animations
enhancedLUT.updateSlotsForFrame(currentFrameTick);
```

### Integration with Graphics Engine
```cpp
// Load enhanced LUT in graphics engine
graphics.loadEnhancedLUT(lut_palette_lut);
graphics.setUseEnhancedLUT(true);

// Configure effects using convenience methods
graphics.setupLUTPulseEffect(0, 0xF800, 8);     // Red pulse
graphics.setupLUTFlashEffect(1, 0x001F, 0x07FF, 2); // Blue flash

// In main loop
uint32_t frameCount = 0;
while (gameRunning) {
    frameCount++;
    graphics.updateLUTForFrame(frameCount);  // Update animations
    
    // Normal rendering...
    graphics.clearBuffers();
    graphics.drawSprite(spriteId, x, y);
    graphics.present();
}
```

## Transparency Model

The system uses **binary transparency** (not RGBA blending):

- **RGB565 color**: Pixel is rendered normally
- **0x0000 (null)**: Pixel is **completely culled** (100% transparent)
- **No alpha blending**: Transparency is all-or-nothing

This matches the user requirement: *"when I say transparent I mean 100% culled, there is not RGBA only RGB or null"*

## Effect Types

### 1. Color Sequences
```cpp
uint16_t waterColors[] = {0x001F, 0x003F, 0x045F, 0x067F, 0x07FF};
enhancedLUT.setSlotSequence(0, waterColors, 5);
```

### 2. Pulse Effects
```cpp
// Automatically generates fade in/out sequence
enhancedLUT.setupPulseEffect(1, 0x07E0, 8);  // Green pulse, 8 steps
```

### 3. Flash Effects
```cpp
// Alternates between two colors
enhancedLUT.setupFlashEffect(2, 0xF800, 0xFFE0, 2);  // Red/Yellow flash
```

### 4. Disabled (Transparent)
```cpp
enhancedLUT.disableSlot(3);  // Slot becomes transparent (0x0000)
```

## Preset Effects

The system includes common effect presets:

```cpp
// Fire effect across all 4 slots (phase-shifted wave)
LUTHelpers::setupFireEffect();

// Water ripple effect  
LUTHelpers::setupWaterEffect();

// Each slot gets different effect
LUTHelpers::setupMixedEffects();

// Warning/alert patterns
LUTHelpers::setupWarningEffects();

// All slots transparent
LUTHelpers::disableAllSlots();
```

## Performance

### Memory Usage
- **Base LUT**: 8192 bytes (64×64 × 2 bytes)
- **Working LUT**: 8192 bytes (copy with current slot values)
- **Slot Data**: ~1KB (4 slots × 16 colors × 16 bytes max)
- **Total**: ~17KB vs previous 32KB (**47% reduction**)

### CPU Impact
- **Animation Update**: O(4) per frame (just update 4 slot values)
- **Color Lookup**: O(1) array access (same as before)
- **Memory Copy**: Only when frame tick changes (not every lookup)

### Frame Rate Compatibility
- Works with app frame rates: 8, 10, 12, 14, 16 FPS
- Independent of system 24 FPS heartbeat
- Each app frame advances animation by 1 step

## Integration Points

### 1. Bootloader Integration
```cpp
// In wisp_bootloader.h
if (graphics->loadEnhancedLUT(lut_palette_lut)) {
    graphics->setUseEnhancedLUT(true);
} else {
    graphics->generateTestLUT(); // Fallback
}
```

### 2. Sprite Rendering Integration
```cpp
// In graphics_engine.h sprite rendering
if (useEnhancedLUT) {
    uint8_t lutX = colorIndex % ENHANCED_LUT_WIDTH;
    uint8_t lutY = colorIndex / ENHANCED_LUT_WIDTH;
    
    if (enhancedLUT.isTransparent(lutX, lutY)) {
        continue;  // 100% culled
    }
    
    finalColor = enhancedLUT.lookupColor(lutX, lutY);
}
```

### 3. App Loader Integration
```cpp
// In app_loader.h
if (header.resources.requiresLUT) {
    loadLUTAssets();  // Configure dynamic effects
}

// In game loop
updateLUTAnimations();  // Update based on frame tick
```

## API Reference

### Core Methods
```cpp
// Load base LUT data
bool loadBaseLUT(const uint16_t* lutData, uint32_t dataSize);

// Configure slot animation
bool setSlotSequence(uint8_t slotIndex, const uint16_t* colors, uint8_t length);

// Disable slot (make transparent)
void disableSlot(uint8_t slotIndex);

// Update animations for current frame
void updateSlotsForFrame(uint32_t frameTick);

// Look up color (includes current slot animations)
uint16_t lookupColor(uint8_t lutX, uint8_t lutY);

// Check if position is transparent
bool isTransparent(uint8_t lutX, uint8_t lutY);
```

### Convenience Methods
```cpp
// Quick effect setup
void setupPulseEffect(uint8_t slot, uint16_t color, uint8_t steps);
void setupFlashEffect(uint8_t slot, uint16_t color1, uint16_t color2, uint8_t rate);
void setupColorCycle(uint8_t slot, const uint16_t* colors, uint8_t count);

// System control
void setEnabled(bool enabled);
bool isEnabled();

// Debug/status
void debugPrintSlots();
void getSlotStatus(uint8_t slot, bool* enabled, uint8_t* length, uint8_t* frame);
```

## Example Use Cases

### 1. Fire/Lava Effects
```cpp
// Animated fire that cycles red→orange→yellow
uint16_t fire[] = {0xF800, 0xF940, 0xFB60, 0xFDA0, 0xFFE0};
enhancedLUT.setSlotSequence(0, fire, 5);
```

### 2. Water/Ice Effects  
```cpp
// Animated water that cycles blue→cyan
uint16_t water[] = {0x001F, 0x003F, 0x045F, 0x067F, 0x07FF};
enhancedLUT.setSlotSequence(1, water, 5);
```

### 3. Energy/Magic Effects
```cpp
// Pulsing energy effect
enhancedLUT.setupPulseEffect(2, 0x07E0, 8);  // Green pulse
```

### 4. Warning/Alert Indicators
```cpp
// Fast red/yellow warning flash
enhancedLUT.setupFlashEffect(3, 0xF800, 0xFFE0, 1);
```

### 5. UI State Indicators
```cpp
// Health: Red pulse when low
if (playerHealth < 25) {
    enhancedLUT.setupPulseEffect(0, 0xF800, 6);
} else {
    enhancedLUT.disableSlot(0);  // Transparent when healthy
}
```

## Migration from Legacy System

### Old System (128×128 LUT)
```cpp
// Legacy 32KB LUT
uint16_t colorLUT[16384];  // 128×128
uint8_t lutX = colorIndex % 128;
uint8_t lutY = colorIndex / 128;
uint16_t color = colorLUT[lutY * 128 + lutX];
```

### New System (64×64 Enhanced LUT)
```cpp
// Enhanced 8KB LUT with dynamic slots
uint8_t lutX = colorIndex % 64;
uint8_t lutY = colorIndex / 64;

if (enhancedLUT.isTransparent(lutX, lutY)) {
    continue;  // Completely culled
}

uint16_t color = enhancedLUT.lookupColor(lutX, lutY);  // Includes animations
```

### Compatibility
- **Color indices 0-4095**: Map to 64×64 LUT (vs 0-16383 for 128×128)
- **Sprites remain unchanged**: Same color index format in sprite data
- **Automatic scaling**: Graphics engine handles LUT size differences
- **Fallback support**: Can disable enhanced LUT to use legacy system

## File Structure

```
wispengine/
├── enhanced_lut_system.h          # Main header with class definition
├── enhanced_lut_system.cpp        # Implementation and presets
├── enhanced_lut_example.cpp       # Usage examples and integration guide
├── lut_palette_data.h             # User's existing 64×64 LUT data
├── src/engine/graphics_engine.h   # Updated to support enhanced LUT
└── src/system/wisp_bootloader.h   # Bootloader integration
```

## Summary

The Enhanced LUT system provides:

1. **47% memory reduction** (8KB vs 32KB)
2. **Dynamic visual effects** with zero CPU overhead
3. **4 animated transparent slots** for special effects
4. **Binary transparency model** (RGB565 or null)
5. **Easy integration** with existing sprite system
6. **Backward compatibility** with legacy LUT system
7. **Rich preset library** for common effects

Perfect for creating engaging visual effects while maintaining the ESP32-C6's memory constraints and performance requirements.
