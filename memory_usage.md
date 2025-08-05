# WispEngine Memory Usage Analysis
## Executive Summary

**Root Cause**: The ESP32-C6 compilation fails due to runtime memory overflow (2MB+ required vs 320KB available) caused by an over-engineered API layer consuming excessive memory through large buffer allocations, complex data structures, and heavyweight libraries.

**Key Finding**: While the source code is 701KB, the **runtime memory footprint** is the real issue, with core systems requiring ~140KB+ just for basic operation - nearly half of ESP32-C6's total RAM.

## Memory Constraint Reality Check

| Platform | Total RAM | Available for App | Flash Storage |
|----------|-----------|-------------------|---------------|
| ESP32-C6 | 320KB | ~250KB (after ESP-IDF) | 4MB |
| ESP32-S3 | 512KB + 16MB PSRAM | ~450KB + 16MB | 16MB |

**The Issue**: ESP32-C6 needs a completely different architecture approach.

## Weighted Memory Footprint Analysis

### 1. LovyanGFX Graphics Library
- **Source Size**: 128.5MB (includes all board variants)
- **Runtime Estimate**: 80-120KB (framebuffers, display drivers, font data)
- **Impact**: Largest single memory consumer
- **Usage**: Essential for display operations across all platforms

### 2. WispEngine Core Systems (Runtime Memory)

#### Audio Engine Buffers (CORRECTED - Mono Only)
```cpp
// CURRENT (Stereo - Over-engineered):
int16_t mixBuffer[AUDIO_BUFFER_SIZE * 2];    // 2048 * 2 * 2 = 8KB
int16_t outputBuffer[AUDIO_BUFFER_SIZE * 2]; // 2048 * 2 * 2 = 8KB  
uint8_t dacBuffer[AUDIO_BUFFER_SIZE * 2];    // 2048 * 2 = 4KB
// Total: 24KB for stereo audio

// PROPOSED (Mono - Appropriate):
int16_t monoBuffer[AUDIO_BUFFER_SIZE];       // 2048 * 2 = 4KB
uint8_t dacBuffer[AUDIO_BUFFER_SIZE];        // 2048 = 2KB
// Total: 8KB for mono audio (67% reduction)
```

#### Database System (LP-SRAM)
```cpp
#define WISP_DB_LP_SRAM_SIZE 16384  // 16KB base allocation
// Partition Layout:
static const uint32_t WISP_DB_ROM_PARTITION_SIZE = 4096;     // 4KB
static const uint32_t WISP_DB_SAVE_PARTITION_SIZE = 4096;    // 4KB  
static const uint32_t WISP_DB_BACKUP_PARTITION_SIZE = 2048;  // 2KB
static const uint32_t WISP_DB_RUNTIME_PARTITION_SIZE = 6144; // 6KB
// Total: 16KB for database partitions
```

#### Graphics System Runtime (SPRITE SLOT ARCHITECTURE)
- **Current Problem**: Sprite layers stored in memory (9 layers × data = ~20KB)
- **Current Problem**: Framebuffer caching and complex pipelines (~15-30KB)
- **Solution**: **Sprite slot system with cached sprites**:
  ```cpp
  struct SpriteSlot {
      uint16_t spriteId;     // Which sprite loaded
      uint8_t* spriteData;   // Cached pixels (16×16 avg)
      bool inUse;            // Slot state
  }; // ~260 bytes per slot
  
  SpriteSlot spriteSlots[24];       // 24 × 260 = ~6.2KB
  SpriteInstance activeSprites[32]; // 32 × 5 = ~160 bytes
  uint16_t colorLUT[256];           // ~512 bytes  
  // Total: ~7KB vs 35KB+ (80% reduction)
  ```
- **Rendering**: Use cached sprite data from slots → Apply LUT → Render to display
- **Performance**: Cache hits avoid flash reads, slots reused across frames

#### UI System Buffers
```cpp
char availableNetworks[10][64];  // 640 bytes
char availableApps[20][64];      // 1280 bytes  
// Plus various 32-byte and 128-byte temporary buffers
// Total: ~2KB for UI data
```

### 3. Source Code Size Breakdown (701KB total)

| Component | Size (KB) | % of Total | Runtime Impact |
|-----------|-----------|------------|----------------|
| **Database Systems** | 94.7 | 13.5% | **High** (16KB+ RAM) |
| **Graphics Engine** | 156.8 | 22.4% | **Critical** (80KB+ RAM) |
| **Audio Engine** | 29.5 | 4.2% | **High** (24KB RAM) |
| **UI Panels** | 100.4 | 14.3% | **Medium** (2-5KB RAM) |
| **App/Loader System** | 87.5 | 12.5% | **Medium** (5-10KB RAM) |
| **System/Platform** | 78.2 | 11.1% | **Low** (1-3KB RAM) |
| **Core/Debug** | 41.8 | 6.0% | **Low** (1-2KB RAM) |
| **Examples/Tests** | 112.3 | 16.0% | **None** (excluded from build) |

### 4. Critical Runtime Memory Estimate

| System | Current RAM | Optimized RAM | ESP32-C6 Impact |
|--------|-------------|---------------|------------------|
| LovyanGFX + Display | 80-120KB | 60-80KB | **19-25% of total RAM** |
| Audio Engine | 24KB | **8KB** | **2.5% of total RAM** |
| Database System | 16KB | **2KB** | **0.6% of total RAM** |
| Sprite/Graphics | 35KB | **7KB** | **2.2% of total RAM** |
| UI Buffers | 2-5KB | 1KB | **0.3% of total RAM** |
| ESP-IDF Overhead | 50-70KB | 50-70KB | **15-22% of total RAM** |
| **OPTIMIZED TOTAL** | **207-270KB** | **128-168KB** | **40-53% usage** |

## The Over-Engineering Problem

### Root Issues Identified:

1. **Audio System Stereo Over-Engineering**
   - Stereo mixing with 24KB buffers for embedded display
   - Complex I2S + DAC dual-output support
   - **Solution**: Mono-only audio with 8KB buffers (67% reduction)

2. **Graphics Layer Storage Anti-Pattern**
   - 9 sprite layers stored in memory with sorting/caching
   - Framebuffer persistence and complex rendering pipelines
   - **Solution**: Sprite slot system with cached sprites:
     - 24 sprite slots (6.2KB) for sprite data caching
     - Sprite instances store only position/scale references
     - Load sprites to slots on-demand (cache misses only)
     - Apply LUT color transformation during render
     - **Memory**: 7KB vs 35KB+ (80% reduction)
     - **Performance**: Cache hits avoid flash reads, 80%+ hit rate expected

3. **Database System Over-Complexity**
   - 4 partition types with 16KB allocation
   - Safe mode, backup systems, encryption support
   - Complex caching and validation layers
   - **Solution**: Simple NVS key-value store ~2KB (87% reduction)

4. **UI System Feature Bloat**
   - Network scanning and management
   - Full system settings with power profiles
   - Complex menu navigation systems
   - **Solution**: Basic button input ~500 bytes

## Platform-Specific Recommendations

### ESP32-C6 (320KB RAM) - **MINIMAL MODE**
**Target: <50KB total engine footprint**

```cpp
// Proposed Minimal Engine Memory Budget:
SimpleGraphics:    7KB   (sprite slot system: 24 slots + instances + LUT)
SimpleStorage:     2KB   (NVS key-value only) 
SimpleAudio:       2KB   (mono buffers, tone generation)
SimpleInput:       500B  (button polling only)
ESP-IDF Base:      50KB  (unavoidable)
LovyanGFX Core:    35KB  (minimal display driver)
App Code Space:    25KB  (for actual applications)
Safety Buffer:     25KB  (heap/stack growth)
Total Used:        ~147KB (46% of 320KB)
Available:         ~173KB (54% remaining for app logic)
```

### ESP32-S3 (512KB + 16MB PSRAM) - **FULL MODE**
**Target: Current architecture acceptable**

- Full feature set with all subsystems
- PSRAM utilization for large buffers
- Complete graphics/audio/database capabilities
- Network and advanced UI features

## Immediate Action Items

### Priority 1: ESP32-C6 Minimal Engine
1. **Implement conditional compilation** for ultra-minimal APIs
2. **Remove heavyweight buffers** (reduce audio buffers to 256 samples)
3. **Eliminate database partitions** (use NVS directly)
4. **Simplify graphics** (remove layers, use direct drawing)

### Priority 2: Memory Profiling
1. **Add heap monitoring** to track actual runtime usage
2. **Implement memory guards** for ESP32-C6 builds
3. **Profile LovyanGFX usage** and optimize for minimal footprint

### Priority 3: Build System Enhancement
1. **Strict conditional compilation** based on target platform
2. **Separate minimal/full engine builds**  
3. **Memory budget validation** during compilation

## Conclusion

The issue is **runtime memory consumption**, not flash storage. The current engine architecture assumes ~200KB+ RAM availability, making it incompatible with ESP32-C6's 320KB constraint. 

**The solution requires architectural changes, not just optimization** - ESP32-C6 needs a fundamentally different, minimal engine while ESP32-S3 can continue using the full-featured version.

The proposed minimal engine targets <50KB total footprint, leaving 70KB+ for application code - a realistic constraint for microcontroller applications.
