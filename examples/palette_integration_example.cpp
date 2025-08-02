// palette_integration_example.cpp
// Example showing how to use converted palette data in Wisp Engine

#include <Arduino.h>
#include "lut_palette_data.h"
#include "src/engine/hybrid_palette_lut_system.h"
#include "src/engine/optimized_sprite_system.h"

// Your converted palette data is now ready to use!
HybridPaletteSystem paletteSystem;
OptimizedSpriteSystem spriteSystem(nullptr); // Graphics engine TBD

void setup() {
    Serial.begin(115200);
    Serial.println("Wisp Engine - Palette Integration Example");
    
    // Load your converted 64×64 LUT
    bool success = paletteSystem.loadColorLUT(lut_palette_lut);
    if (success) {
        Serial.println("✓ LUT palette loaded successfully!");
        Serial.print("Memory usage: ");
        Serial.print(paletteSystem.getMemoryUsage());
        Serial.println(" bytes");
    } else {
        Serial.println("✗ Failed to load LUT palette");
        return;
    }
    
    // Show palette stats
    paletteSystem.printStats();
    
    // Example usage: Get colors from different LUT positions
    Serial.println("\nSample colors from your LUT:");
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t x = i * 8;     // Sample every 8th column
        uint8_t y = i * 8;     // Sample every 8th row
        
        uint16_t color = paletteSystem.getLutColor(x, y);
        
        // Convert RGB565 back to RGB888 for display
        uint8_t r = ((color >> 11) & 0x1F) * 255 / 31;
        uint8_t g = ((color >> 5) & 0x3F) * 255 / 63;
        uint8_t b = (color & 0x1F) * 255 / 31;
        
        Serial.print("LUT[");
        Serial.print(x);
        Serial.print(",");
        Serial.print(y);
        Serial.print("] = 0x");
        Serial.print(color, HEX);
        Serial.print(" → RGB(");
        Serial.print(r);
        Serial.print(",");
        Serial.print(g);
        Serial.print(",");
        Serial.print(b);
        Serial.println(")");
    }
    
    // Example: Create basic palettes for sprites
    Serial.println("\nCreating sprite palettes...");
    
    // Create a Game Boy-style 4-color palette using LUT colors
    uint16_t gbPalette[4] = {
        paletteSystem.getLutColor(0, 0),    // Darkest (from LUT corner)
        paletteSystem.getLutColor(20, 20),  // Dark gray
        paletteSystem.getLutColor(40, 40),  // Light gray  
        paletteSystem.getLutColor(60, 60)   // Lightest (from LUT corner)
    };
    
    // If we had a pure palette system running alongside:
    // paletteSystem.loadPalette(0, gbPalette, 4);
    
    Serial.println("✓ Game Boy palette created from LUT");
    
    // Example: Advanced color mixing
    Serial.println("\nAdvanced color mixing examples:");
    
    // Get a base sprite color (pretend this comes from a palette)
    uint16_t baseColor = paletteSystem.getLutColor(32, 16); // Some mid-range color
    
    // Mix with different LUT positions for effects
    uint16_t fireEffect = paletteSystem.getLutColor(48, 8);   // Reddish area
    uint16_t waterEffect = paletteSystem.getLutColor(16, 48); // Bluish area
    uint16_t lightEffect = paletteSystem.getLutColor(56, 56); // Bright area
    
    Serial.print("Base color: 0x");
    Serial.println(baseColor, HEX);
    Serial.print("Fire effect: 0x");
    Serial.println(fireEffect, HEX);
    Serial.print("Water effect: 0x");
    Serial.println(waterEffect, HEX);
    Serial.print("Light effect: 0x");
    Serial.println(lightEffect, HEX);
    
    Serial.println("\n🎨 Your custom palette is ready for game development!");
    Serial.println("Total memory saved vs 128×128 LUT: 24KB (75% reduction)");
}

void loop() {
    // In a real game loop, you would:
    
    // 1. Update animated palette colors
    // paletteSystem.updateAnimations();
    
    // 2. Get colors for sprite rendering
    // uint16_t spriteColor = paletteSystem.getLutColor(lutX, lutY);
    
    // 3. Blend colors for special effects
    // uint16_t effectColor = paletteSystem.getBlendedColor(paletteId, colorIndex, lutX, lutY);
    
    // 4. Use colors in sprite system
    // spriteSystem.render();
    
    delay(100); // Simple delay for this example
}

// Example of how this integrates with sprite rendering
void renderSpriteWithPalette(uint8_t spriteId, int16_t x, int16_t y) {
    // Pseudo-code for sprite rendering with palette system
    
    /*
    // Get sprite pixel data (palette indices)
    uint8_t* spriteData = getSpriteData(spriteId);
    uint8_t width = getSpriteWidth(spriteId);
    uint8_t height = getSpriteHeight(spriteId);
    
    for (uint8_t py = 0; py < height; py++) {
        for (uint8_t px = 0; px < width; px++) {
            uint8_t paletteIndex = spriteData[py * width + px];
            
            if (paletteIndex == 0) continue; // Transparent
            
            // Option 1: Direct palette lookup
            uint16_t color = paletteSystem.getColor(0, paletteIndex);
            
            // Option 2: LUT-based color mixing
            uint8_t lutX = (x + px) & 63;  // Wrap to LUT bounds
            uint8_t lutY = (y + py) & 63;
            uint16_t color = paletteSystem.getLutColor(lutX, lutY);
            
            // Option 3: Advanced blending
            uint16_t color = paletteSystem.getBlendedColor(0, paletteIndex, lutX, lutY);
            
            // Draw pixel at (x+px, y+py) with final color
            setPixel(x + px, y + py, color);
        }
    }
    */
}

/*
Memory Usage Comparison:

Old system (128×128 LUT):
- Color LUT: 32,768 bytes (32KB)
- Total graphics: ~194KB 
- Game logic: ~126KB

New system (64×64 LUT from your PNG):
- Color LUT: 8,192 bytes (8KB) ✓
- Total graphics: ~170KB ✓
- Game logic: ~150KB ✓

Savings: 24KB reclaimed for game logic (19% more memory!)

Your custom 64×64 palette provides:
- 4,096 unique color combinations
- Smooth gradients and transitions
- Real-time color effects
- 75% memory savings vs original system
- Perfect balance of features and efficiency

This is exactly the optimization that makes retro-style games possible
on the ESP32-C6 while still providing modern visual effects!
*/
