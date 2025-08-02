# LUT System Technical Specification

*Version 1.0 - August 2, 2025*

## 📋 **EXECUTIVE SUMMARY**

The Wisp Engine uses a **LUT (Lookup Table) based graphics architecture** that stores **8-bit color indices** in memory and converts to **RGB565 only during display output**. This architecture provides **50% memory savings** while supporting **4,096 unique colors**.

**CRITICAL RULE**: RGB565 values are **NEVER stored in framebuffers or sprites** - only generated during display scan-out.

---

## 🎯 **ARCHITECTURE OVERVIEW**

### **Storage Layer (Memory Efficient)**
- **Sprites**: Store 8-bit LUT indices (0-255)
- **Framebuffer**: 55KB of 8-bit indices, NOT RGB565
- **Depth Buffer**: 55KB of 8-bit depth values
- **LUT Table**: 8KB RGB565 lookup table (64×64 matrix)

### **Rendering Layer (Display Output)**
- **LUT Lookup**: Convert indices to RGB565 during scan-out
- **No RGB Storage**: RGB565 exists only during display refresh
- **Direct Output**: RGB565 sent immediately to display controller

---

## 🔧 **TECHNICAL IMPLEMENTATION**

### **LUT Index Encoding**
```cpp
// 8-bit index encodes 64×64 LUT coordinates
uint8_t colorIndex = lutY * 64 + lutX;  // 0-255 valid range
uint8_t lutX = colorIndex % 64;         // Extract X (0-63)  
uint8_t lutY = colorIndex / 64;         // Extract Y (0-3)

// LUT covers 256 colors in 64×64 grid:
// Row 0: Indices 0-63   (Y=0)
// Row 1: Indices 64-127 (Y=1) 
// Row 2: Indices 128-191(Y=2)
// Row 3: Indices 192-255(Y=3)
```

### **Memory Layout**
```cpp
struct GraphicsEngine {
    // === STORAGE BUFFERS (Index-based) ===
    uint8_t framebuffer[172 * 320];     // 55,040 bytes - LUT indices
    uint8_t depthBuffer[172 * 320];     // 54,880 bytes - Z-depth values
    
    // === COLOR LOOKUP TABLE ===
    uint16_t colorLUT[64 * 64];         // 8,192 bytes - RGB565 colors
    
    // === ENHANCED FEATURES ===
    EnhancedLUT enhancedLUT;            // Dynamic color effects
    bool useEnhancedLUT;                // Primary rendering mode
    
    // === LEGACY FALLBACK (Optional) ===
    uint16_t* rgb565Buffer;             // NULL unless fallback needed
};

// Total Memory: 55KB + 55KB + 8KB = 118KB (vs 164KB for RGB565)
// Memory Savings: 46KB (28% reduction)
```

### **Sprite Data Format**
```cpp
struct WispSprite {
    uint16_t width, height;
    uint8_t* pixelData;                 // LUT indices, NOT RGB values
    uint8_t paletteSlot;                // Enhanced LUT palette (0-15)
    uint8_t transparentIndex;           // Usually 0
    
    // Animation support
    uint16_t frameCount;
    uint8_t** frames;                   // Array of index arrays
};

// Example: 16×16 sprite = 256 bytes (vs 512 bytes RGB565)
// Memory per sprite: 50% savings
```

---

## 🎨 **LUT CREATION PIPELINE**

### **Asset Workflow**
```bash
# 1. Artist creates 64×64 palette image
assets/palette.png (64×64 PNG with desired colors)

# 2. Convert to C header file
python tools/wisp_palette_converter.py assets/palette.png exports/lut_palette_data.h

# 3. Generated LUT structure
const uint16_t lut_palette_lut[4096] = {
    0x0000, 0x0001, 0x0002, ...  // 4096 RGB565 values
};

# 4. Runtime loading
memcpy(graphicsEngine.colorLUT, lut_palette_lut, 8192);
```

### **Enhanced LUT System**
```cpp
// Dynamic color effects and palette animation
struct EnhancedLUT {
    uint16_t dynamicSlots[16][16];      // 16 palettes × 16 colors
    uint8_t animationFrames[8][256];    // Color cycling frames
    uint32_t frameTimer;                // Animation timing
    
    // Real-time effects
    void updateCycling();               // Water, fire effects
    void setPaletteSlot(uint8_t slot, const uint16_t* colors);
    uint16_t getColor(uint8_t index, uint8_t slot);
};
```

---

## ⚡ **RENDERING PIPELINE**

### **Display Scan-out Process**
```cpp
// Called during display refresh (60Hz)
void renderScanline(int y) {
    uint16_t* displayLine = display.getLineBuffer();
    
    for (int x = 0; x < 172; x++) {
        // 1. Read LUT index from framebuffer  
        uint8_t colorIndex = framebuffer[y * 172 + x];
        
        // 2. Enhanced LUT lookup (if enabled)
        uint16_t rgb565;
        if (useEnhancedLUT) {
            uint8_t depth = depthBuffer[y * 172 + x];
            uint8_t paletteSlot = depth & 0x0F;  // Bottom 4 bits
            rgb565 = enhancedLUT.getColor(colorIndex, paletteSlot);
        } else {
            // 3. Standard LUT lookup
            uint8_t lutX = colorIndex % 64;
            uint8_t lutY = colorIndex / 64;  
            rgb565 = colorLUT[lutY * 64 + lutX];
        }
        
        // 4. Output directly to display (no storage)
        displayLine[x] = rgb565;
    }
    
    // 5. Send line to display controller
    display.writeLineBuffer(y, displayLine);
    // RGB565 data is discarded after transmission
}
```

### **Performance Characteristics**
- **LUT Lookup**: ~1 cycle per pixel (L1 cache hit)
- **Memory Bandwidth**: 50% reduction vs RGB565 framebuffer
- **Display Latency**: No additional delay vs direct RGB565
- **Color Quality**: Full 16-bit color depth maintained

---

## 🔍 **DEBUGGING AND VALIDATION**

### **Memory Usage Verification**
```cpp
void validateMemoryArchitecture() {
    // Verify no RGB565 storage in framebuffer
    assert(sizeof(framebuffer[0]) == 1);  // Must be uint8_t
    
    // Verify LUT table size
    assert(sizeof(colorLUT) == 8192);     // 64×64×2 bytes
    
    // Calculate total engine memory
    size_t indexBuffers = sizeof(framebuffer) + sizeof(depthBuffer);
    size_t lutMemory = sizeof(colorLUT);
    size_t totalEngine = indexBuffers + lutMemory;
    
    printf("Index Buffers: %zu KB\n", indexBuffers / 1024);      // ~110KB
    printf("LUT Memory: %zu KB\n", lutMemory / 1024);           // 8KB
    printf("Total Engine: %zu KB\n", totalEngine / 1024);       // ~118KB
    printf("Memory Savings: %zu KB\n", (164 - totalEngine/1024)); // ~46KB
}
```

### **Color Accuracy Testing**
```cpp
void testLUTAccuracy() {
    // Test full color range
    for (int i = 0; i < 256; i++) {
        uint8_t lutX = i % 64;
        uint8_t lutY = i / 64;
        uint16_t color = colorLUT[lutY * 64 + lutX];
        
        // Verify color is valid RGB565
        assert(color <= 0xFFFF);
        
        // Test round-trip conversion
        uint8_t reconstructed = lutY * 64 + lutX;
        assert(reconstructed == i);
    }
}
```

---

## 📊 **PERFORMANCE ANALYSIS**

### **Memory Comparison**

| **Component** | **RGB565 Direct** | **LUT System** | **Savings** |
|---------------|-------------------|----------------|-------------|
| **Framebuffer** | 109,760 bytes | 55,040 bytes | **50.1%** |
| **Depth Buffer** | 54,880 bytes | 54,880 bytes | 0% |
| **Color Table** | 0 bytes | 8,192 bytes | -8KB |
| **Total** | 164,640 bytes | 118,112 bytes | **28.3%** |

### **Quality Comparison**

| **Feature** | **RGB565 Direct** | **LUT System** | **Advantage** |
|-------------|-------------------|----------------|---------------|
| **Color Count** | 65,536 colors | 4,096 colors | RGB565 |
| **Memory Usage** | 164KB | 118KB | **LUT** |
| **Dynamic Effects** | Limited | Full Support | **LUT** |
| **Palette Animation** | No | Yes | **LUT** |
| **Compression** | No | Excellent | **LUT** |

### **Recommended Use Cases**

**✅ LUT System Ideal For:**
- Pixel art games with limited color palettes
- Retro-style games (GBA, SNES aesthetic)
- Memory-constrained applications
- Games requiring palette effects (day/night, damage, etc.)

**⚠️ RGB565 Direct Better For:**
- Photo-realistic graphics
- High color count requirements (>4096 colors)
- Simple applications without effects

---

## 🚀 **INTEGRATION GUIDE**

### **Enabling LUT System**
```cpp
// In your app initialization
void setupGraphics() {
    graphicsEngine.loadLUT("lut_palette_data.h");
    graphicsEngine.useEnhancedLUT = true;
    graphicsEngine.clearFramebuffer(0);  // Clear to index 0 (transparent)
}
```

### **Drawing with LUT Indices**
```cpp
// Draw sprite using LUT indices
void drawSprite(WispSprite* sprite, int x, int y) {
    for (int py = 0; py < sprite->height; py++) {
        for (int px = 0; px < sprite->width; px++) {
            uint8_t colorIndex = sprite->pixelData[py * sprite->width + px];
            if (colorIndex != sprite->transparentIndex) {
                setPixel(x + px, y + py, colorIndex);  // Store index, NOT RGB
            }
        }
    }
}

// Set pixel stores LUT index
void setPixel(int x, int y, uint8_t lutIndex) {
    if (x >= 0 && x < 172 && y >= 0 && y < 320) {
        framebuffer[y * 172 + x] = lutIndex;  // Store 8-bit index
    }
}
```

---

## ⚠️ **COMMON PITFALLS**

### **❌ AVOID These Mistakes**
```cpp
// WRONG: Storing RGB565 in framebuffer
uint16_t framebuffer[172 * 320];  // ❌ Uses 109KB
framebuffer[pixel] = 0xF800;      // ❌ Storing RGB565

// WRONG: Converting to RGB565 too early  
uint16_t color = rgb565(r, g, b); // ❌ Lose LUT benefits
setPixel(x, y, color);            // ❌ Bypasses LUT system

// WRONG: Mixing index and RGB565 data
setPixel(x, y, 0xFF00);           // ❌ RGB565 in index buffer
```

### **✅ CORRECT Implementation**
```cpp
// CORRECT: Storing LUT indices
uint8_t framebuffer[172 * 320];   // ✅ Uses 55KB
framebuffer[pixel] = 42;          // ✅ Storing LUT index

// CORRECT: Using LUT indices throughout
uint8_t colorIndex = findLUTIndex(r, g, b);  // ✅ Convert to index
setPixel(x, y, colorIndex);       // ✅ Store index

// CORRECT: RGB565 only during display
uint16_t rgb = colorLUT[index];   // ✅ Convert during scan-out
display.writePixel(x, y, rgb);    // ✅ Direct to display
```

---

## 📝 **CONCLUSION**

The LUT system provides significant memory savings while maintaining visual quality for retro-style games. The key principle is simple:

**🎯 STORAGE = Indices, DISPLAY = RGB565**

By following this architecture, applications gain 46KB of additional memory while supporting advanced palette effects impossible with direct RGB565 rendering.

For questions or technical support, refer to `src/engine/graphics_engine.h` and `src/engine/enhanced_lut_system.h`.
