# Memory Analysis: ESP32-C6 Engine Optimization

## Current Memory Usage (Problematic)

### Large Components Being Compiled:
- **Sprite Layers System**: 20KB+ (553 lines)
  - 9 complex sprite layers
  - Advanced sorting/rendering
  - Multiple sprite types
  
- **Database System**: 20KB+ (618 lines)
  - Partitioned database with caching
  - Multiple partition types
  - Complex validation logic
  
- **Graphics Engine**: 15KB+ 
  - LUT color transformation system
  - Palette management
  - Advanced effects pipeline

- **Audio Settings UI**: 14KB+
  - Complex audio processing interface

**TOTAL ESTIMATED**: ~70KB+ just for engine code
**PLUS**: LovyanGFX library (~100KB+)
**PLUS**: ESP-IDF framework components
**RESULT**: 200KB+ flash usage, likely causing 2MB+ memory overflow

## Key Architecture Issues Identified

### 1. Audio System Over-Engineering
- **Current**: Stereo mixing with 24KB buffers (mixBuffer + outputBuffer + dacBuffer)
- **Problem**: ESP32-C6 doesn't need stereo complexity
- **Solution**: Mono audio with 8KB total buffers (67% reduction)

### 2. Graphics Layer Storage Anti-Pattern  
- **Current**: 9 sprite layers stored in memory with complex sorting
- **Problem**: Massive memory consumption for layer data
- **Solution**: Compute-per-frame rendering:
  - Store only sprite references (ID, x, y, scale)
  - Recompute entire frame each cycle from sprite data + LUT
  - No layer caching, no framebuffer storage beyond display buffer
  - Use sprite atlas + LUT table for efficient rendering

## ESP32-C6 Constraints

- **SRAM**: 320KB total (shared between heap, stack, static data)
- **Flash**: 4MB total 
- **Practical Limits**: 
  - Application code: ~200KB max
  - Runtime heap: ~200KB max
  - Framework overhead: ~100KB

## Recommended Minimal API for ESP32-C6

### Graphics (Target: <2KB)
```cpp
// COMPUTE-PER-FRAME: No layer storage, recompute each frame
// Only store sprite references + LUT color table
// Immediate mode rendering to display buffer
drawSprite(spriteId, x, y), drawRect(), drawText()
clear(), display()
// Sprite data stored in flash, rendered on-demand
```

### Storage (Target: <1KB)  
```cpp
// Simple key-value storage
saveValue(), loadValue(), hasKey()
```

### Audio (Target: <500B)
```cpp
// MONO ONLY - no stereo buffers needed
// Basic tones only
playTone(), playBeep(), silence()
// Single channel audio: ~8KB instead of 24KB
```

### Graphics (Target: <8KB)
```cpp
// SPRITE SLOT SYSTEM: Limited slots with cached data
// 24 sprite slots × ~260 bytes = ~6.2KB
// Position instances × ~5 bytes = ~160 bytes
// LUT color table = ~512 bytes
// Total: ~7KB for sprite system
drawSprite(spriteId, x, y), drawRect(), drawText()
clear(), display()
// Sprites cached in RAM slots, loaded from flash on-demand
```

### Input (Target: <500B)
```cpp
// Basic button reading
isButtonPressed(), wasButtonJustPressed()
```

**TOTAL TARGET**: ~10KB engine code vs current 70KB+

## Implementation Strategy

1. **Conditional Compilation**: Use `#if defined(PLATFORM_C6)` extensively
2. **Separate Engine Classes**: MinimalEngine vs FullEngine  
3. **Feature Flags**: Runtime memory profiling levels
4. **Library Selection**: Consider alternatives to LovyanGFX for C6

## Memory Savings Analysis

### Audio Optimization
- **Current**: 24KB stereo buffers (mixBuffer + outputBuffer + dacBuffer)
- **Optimized**: 8KB mono buffers  
- **Savings**: -16KB (67% reduction)

### Graphics Architecture Change
- **Remove Sprite Layer Storage**: -20KB (9 layers eliminated)
- **Remove Framebuffer Caching**: -15KB (no persistent buffers)
- **Add Sprite Slot System**: +7KB (24 slots × ~260 bytes each)
- **Net Savings**: -28KB total (78% reduction)

### Sprite Slot Benefits:
- **Performance**: No flash reads per frame (cached sprites)
- **Memory Efficient**: Only 24 sprites max vs unlimited layer data
- **Predictable**: Fixed memory footprint
- **Flexible**: Slots can be reassigned as needed

### Other Reductions
- **Remove Database System**: -20KB  
- **Remove Audio UI**: -14KB
- **Use Minimal LovyanGFX config**: -50KB

**TOTAL SAVINGS**: ~128KB (should fix the 2MB overflow)

## Sprite Slot Management System

### Memory Allocation:
```cpp
// 24 sprite slots × 260 bytes = 6.2KB
// Assumes average 16×16 sprites (256 bytes + metadata)
// Larger sprites (32×32 = 1KB) would reduce slot count accordingly
```

### Slot Management:
```cpp
class SpriteSlotManager {
    SpriteSlot slots[24];
    uint8_t nextSlot = 0;
    
    uint8_t loadSprite(uint16_t spriteId) {
        // Check if already loaded
        for (int i = 0; i < 24; i++) {
            if (slots[i].inUse && slots[i].spriteId == spriteId) {
                return i; // Cache hit
            }
        }
        
        // Load to next available slot (LRU eviction)
        uint8_t slotId = nextSlot++;
        if (nextSlot >= 24) nextSlot = 0;
        
        loadSpriteFromFlash(spriteId, &slots[slotId]);
        return slotId;
    }
};
```

### Performance Characteristics:
- **Cache Hit**: Instant rendering (sprite already in RAM)
- **Cache Miss**: One flash read to load sprite to slot
- **Typical Game**: 80%+ cache hit rate (sprites reused across frames)

## Compute-Per-Frame Graphics Architecture

### Current (Memory Heavy):
```cpp
// BAD: Store all layer data in RAM
LayerData layers[9];           // ~20KB
SpriteBuffer framebuffer;      // ~15KB  
ColorPalette cachedPalettes;   // ~10KB
```

### Proposed (Sprite Slot System):
```cpp
// GOOD: Limited sprite slots with cached data + position references
struct SpriteSlot {
    uint16_t spriteId;     // Which sprite is loaded
    uint8_t* spriteData;   // Cached sprite pixels (16x16 = 256 bytes avg)
    bool inUse;            // Slot occupied
}; // ~260 bytes per slot

struct SpriteInstance {
    uint8_t slotId;        // Which sprite slot to use
    int16_t x, y;          // Position
    uint8_t scale;         // Size modifier
}; // ~5 bytes per instance

SpriteSlot spriteSlots[24];        // 24 * 260 = ~6.2KB sprite cache
SpriteInstance activeSprites[32]; // 32 * 5 = ~160 bytes positions
uint16_t colorLUT[256];           // ~512 bytes for color mapping
// Total: ~7KB vs 45KB+ (85% reduction)
```

### Rendering Flow:
1. **Load sprites to slots** (on-demand, cache misses only)
2. **Clear display buffer** (direct to LGFX)
3. **For each active sprite instance**:
   - Use cached sprite data from slot
   - Apply LUT color transformation  
   - Render directly to display buffer
4. **Present buffer** to display
5. **Sprite slots persist** across frames for performance

## Recommendation

For ESP32-C6: Use ultra-minimal engine with basic functionality only.
For ESP32-S3: Keep full-featured engine with all capabilities.
