# 64×64 Palette LUT Analysis - Sweet Spot Optimization

## Current vs 64×64 LUT Comparison

### Memory Usage:
- **128×128 LUT**: 128 × 128 × 2 = **32,768 bytes (32KB)**
- **64×64 LUT**: 64 × 64 × 2 = **8,192 bytes (8KB)** 
- **Memory savings**: 32KB → 8KB = **75% reduction!**

### Index Range Benefits:
- **128×128**: Requires 7-bit indices (0-127) for each dimension
- **64×64**: Requires 6-bit indices (0-63) for each dimension
- **Combined index**: Can pack both X,Y into single 12-bit value (6+6 bits)

## Data Structure Optimizations

### Compact Index Packing (64×64):
```cpp
// Instead of separate x,y coordinates:
struct SpriteOld {
    uint8_t lutX, lutY;  // 2 bytes for LUT coordinates
};

// Pack both into 12 bits:
struct SpriteCompact {
    uint16_t lutIndex : 12;  // 6 bits X + 6 bits Y = 12 bits total
    uint16_t flags : 4;      // 4 remaining bits for flags
};

// Extract coordinates:
uint8_t lutX = lutIndex & 0x3F;        // Lower 6 bits
uint8_t lutY = (lutIndex >> 6) & 0x3F; // Upper 6 bits
```

### Memory Comparison by Profile:

#### MINIMAL Profile Options:
1. **No LUT** (pure palettes): 128 bytes
2. **32×32 LUT**: 2KB + palettes = 2.1KB
3. **64×64 LUT**: 8KB + palettes = 8.1KB

#### BALANCED Profile Options:
1. **Pure palettes**: 512 bytes  
2. **64×64 LUT**: 8KB + palettes = 8.5KB
3. **Original 128×128**: 32KB (too big!)

#### FULL Profile Options:
1. **Pure palettes**: 2KB
2. **64×64 LUT**: 8KB + palettes = 10KB  
3. **Original 128×128**: 32KB + palettes = 34KB
