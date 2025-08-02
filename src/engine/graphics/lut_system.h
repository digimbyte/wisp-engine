// Enhanced LUT/Palette System with Dynamic Transparent Slots
// Supports 4 dynamic transparent slots at LUT positions (64, 61-64)
// These slots cycle through user-defined color sequences each app frame tick

#pragma once
#include <stdint.h>
#include "../system/definitions.h"

// Enhanced LUT dimensions (matching existing data)
#define ENHANCED_LUT_WIDTH 64
#define ENHANCED_LUT_HEIGHT 64
#define ENHANCED_LUT_SIZE 4096

// Dynamic transparent slot configuration
#define TRANSPARENT_SLOT_ROW 64        // Row 64 (last row, 0-indexed: row 63)
#define TRANSPARENT_SLOT_START_COL 61  // Columns 61-64
#define TRANSPARENT_SLOT_COUNT 4       // 4 slots total
#define MAX_SEQUENCE_LENGTH 16         // Maximum colors in animation sequence

// Transparent slot positions in LUT array (row 63, cols 61-64)
#define SLOT_0_INDEX ((63 * ENHANCED_LUT_WIDTH) + 61)  // Position (63, 61)
#define SLOT_1_INDEX ((63 * ENHANCED_LUT_WIDTH) + 62)  // Position (63, 62) 
#define SLOT_2_INDEX ((63 * ENHANCED_LUT_WIDTH) + 63)  // Position (63, 63)
#define SLOT_3_INDEX ((63 * ENHANCED_LUT_WIDTH) + 60)  // Position (63, 60) - wraps to col 60

// Animation sequence for transparent slots
struct TransparentSlotSequence {
    uint16_t colors[MAX_SEQUENCE_LENGTH];  // RGB565 color sequence
    uint8_t length;                        // Number of colors in sequence (0 = disabled/null)
    uint8_t currentFrame;                  // Current position in sequence
    bool enabled;                          // Whether this slot is active
};

// Enhanced LUT system with dynamic transparency
class EnhancedLUTSystem {
private:
    uint16_t baseLUT[ENHANCED_LUT_SIZE];               // Base 64×64 LUT data
    TransparentSlotSequence slots[TRANSPARENT_SLOT_COUNT]; // 4 dynamic slots
    uint16_t workingLUT[ENHANCED_LUT_SIZE];            // Working copy with current slot values
    uint32_t lastFrameTick;                            // Last app frame when slots were updated
    bool systemEnabled;
    
public:
    EnhancedLUTSystem() {
        systemEnabled = true;
        lastFrameTick = 0;
        
        // Initialize all slots as disabled/transparent (null)
        for (int i = 0; i < TRANSPARENT_SLOT_COUNT; i++) {
            slots[i].enabled = false;
            slots[i].length = 0;
            slots[i].currentFrame = 0;
            memset(slots[i].colors, 0, sizeof(slots[i].colors));
        }
        
        // Clear LUT arrays
        memset(baseLUT, 0, sizeof(baseLUT));
        memset(workingLUT, 0, sizeof(workingLUT));
    }
    
    // Load base LUT data (64×64 format)
    bool loadBaseLUT(const uint16_t* lutData, uint32_t dataSize = ENHANCED_LUT_SIZE) {
        if (!lutData || dataSize != ENHANCED_LUT_SIZE) {
            Serial.println("ERROR: Invalid LUT data size");
            return false;
        }
        
        // Copy base LUT
        memcpy(baseLUT, lutData, ENHANCED_LUT_SIZE * sizeof(uint16_t));
        
        // Initialize working LUT with base data
        memcpy(workingLUT, baseLUT, ENHANCED_LUT_SIZE * sizeof(uint16_t));
        
        // Set transparent slots to null (0x0000) initially
        workingLUT[SLOT_0_INDEX] = 0x0000;  // Transparent/null
        workingLUT[SLOT_1_INDEX] = 0x0000;
        workingLUT[SLOT_2_INDEX] = 0x0000;
        workingLUT[SLOT_3_INDEX] = 0x0000;
        
        Serial.println("Enhanced LUT: Base LUT loaded (64×64)");
        return true;
    }
    
    // Configure animation sequence for a transparent slot
    bool setSlotSequence(uint8_t slotIndex, const uint16_t* colorSequence, uint8_t sequenceLength) {
        if (slotIndex >= TRANSPARENT_SLOT_COUNT || !colorSequence || sequenceLength == 0 || sequenceLength > MAX_SEQUENCE_LENGTH) {
            Serial.println("ERROR: Invalid slot sequence parameters");
            return false;
        }
        
        TransparentSlotSequence& slot = slots[slotIndex];
        
        // Copy color sequence
        memcpy(slot.colors, colorSequence, sequenceLength * sizeof(uint16_t));
        slot.length = sequenceLength;
        slot.currentFrame = 0;
        slot.enabled = true;
        
        Serial.print("Enhanced LUT: Slot ");
        Serial.print(slotIndex);
        Serial.print(" configured with ");
        Serial.print(sequenceLength);
        Serial.println(" colors");
        
        return true;
    }
    
    // Disable a transparent slot (set to null/transparent)
    void disableSlot(uint8_t slotIndex) {
        if (slotIndex >= TRANSPARENT_SLOT_COUNT) return;
        
        slots[slotIndex].enabled = false;
        slots[slotIndex].length = 0;
        
        // Set slot to transparent in working LUT
        uint16_t lutIndex = getSlotLUTIndex(slotIndex);
        workingLUT[lutIndex] = 0x0000;  // Null/transparent
        
        Serial.print("Enhanced LUT: Slot ");
        Serial.print(slotIndex);
        Serial.println(" disabled (transparent)");
    }
    
    // Update slots based on app frame tick (called each frame)
    void updateSlotsForFrame(uint32_t currentFrameTick) {
        if (!systemEnabled || currentFrameTick == lastFrameTick) {
            return;  // No update needed
        }
        
        lastFrameTick = currentFrameTick;
        
        // Update each enabled slot
        for (uint8_t i = 0; i < TRANSPARENT_SLOT_COUNT; i++) {
            if (!slots[i].enabled || slots[i].length == 0) {
                continue;
            }
            
            TransparentSlotSequence& slot = slots[i];
            
            // Advance to next frame in sequence
            slot.currentFrame = (slot.currentFrame + 1) % slot.length;
            
            // Update working LUT with current color
            uint16_t lutIndex = getSlotLUTIndex(i);
            workingLUT[lutIndex] = slot.colors[slot.currentFrame];
        }
    }
    
    // Get current working LUT (includes animated slot values)
    const uint16_t* getCurrentLUT() const {
        return workingLUT;
    }
    
    // Get LUT size for compatibility
    uint32_t getLUTSize() const {
        return ENHANCED_LUT_SIZE;
    }
    
    // Get LUT dimensions
    void getLUTDimensions(uint16_t* width, uint16_t* height) const {
        *width = ENHANCED_LUT_WIDTH;
        *height = ENHANCED_LUT_HEIGHT;
    }
    
    // Lookup color from current LUT (with slot animations)
    uint16_t lookupColor(uint8_t lutX, uint8_t lutY) const {
        if (lutX >= ENHANCED_LUT_WIDTH || lutY >= ENHANCED_LUT_HEIGHT) {
            return 0x0000;  // Out of bounds = transparent
        }
        
        uint16_t index = lutY * ENHANCED_LUT_WIDTH + lutX;
        return workingLUT[index];
    }
    
    // Check if a LUT position represents transparency (null state)
    bool isTransparent(uint8_t lutX, uint8_t lutY) const {
        uint16_t color = lookupColor(lutX, lutY);
        return (color == 0x0000);  // RGB565 null = completely culled
    }
    
    // Check if a LUT position is a dynamic slot
    bool isDynamicSlot(uint8_t lutX, uint8_t lutY) const {
        if (lutY != 63) return false;  // Only row 63 contains slots
        return (lutX >= 60 && lutX <= 63);  // Slots at columns 60-63
    }
    
    // Get slot index for LUT position (if it's a dynamic slot)
    int8_t getSlotForPosition(uint8_t lutX, uint8_t lutY) const {
        if (!isDynamicSlot(lutX, lutY)) return -1;
        
        // Map columns to slot indices
        switch (lutX) {
            case 60: return 3;  // Slot 3
            case 61: return 0;  // Slot 0  
            case 62: return 1;  // Slot 1
            case 63: return 2;  // Slot 2
            default: return -1;
        }
    }
    
    // Enable/disable the entire system
    void setEnabled(bool enabled) {
        systemEnabled = enabled;
        if (!enabled) {
            // Reset to base LUT
            memcpy(workingLUT, baseLUT, ENHANCED_LUT_SIZE * sizeof(uint16_t));
        }
    }
    
    // Check if system is enabled
    bool isEnabled() const {
        return systemEnabled;
    }
    
    // Get slot status for debugging
    void getSlotStatus(uint8_t slotIndex, bool* enabled, uint8_t* length, uint8_t* currentFrame) const {
        if (slotIndex >= TRANSPARENT_SLOT_COUNT) {
            *enabled = false;
            *length = 0;
            *currentFrame = 0;
            return;
        }
        
        const TransparentSlotSequence& slot = slots[slotIndex];
        *enabled = slot.enabled;
        *length = slot.length;
        *currentFrame = slot.currentFrame;
    }
    
    // Debug: Print current slot states
    void debugPrintSlots() const {
        Serial.println("Enhanced LUT - Slot Status:");
        for (uint8_t i = 0; i < TRANSPARENT_SLOT_COUNT; i++) {
            const TransparentSlotSequence& slot = slots[i];
            Serial.print("  Slot ");
            Serial.print(i);
            Serial.print(": ");
            if (slot.enabled) {
                Serial.print("Enabled, ");
                Serial.print(slot.length);
                Serial.print(" colors, frame ");
                Serial.print(slot.currentFrame);
                Serial.print(" (color: 0x");
                Serial.print(slot.colors[slot.currentFrame], HEX);
                Serial.println(")");
            } else {
                Serial.println("Disabled (transparent)");
            }
        }
    }
    
    // Quick setup methods for common patterns
    void setupPulseEffect(uint8_t slotIndex, uint16_t baseColor, uint8_t steps = 8) {
        if (slotIndex >= TRANSPARENT_SLOT_COUNT || steps == 0 || steps > MAX_SEQUENCE_LENGTH) return;
        
        uint16_t sequence[MAX_SEQUENCE_LENGTH];
        
        // Create fade in/out pulse
        for (uint8_t i = 0; i < steps; i++) {
            float intensity = (sin((i * PI * 2) / steps) + 1.0f) / 2.0f;  // 0.0 to 1.0
            sequence[i] = scaleColor(baseColor, intensity);
        }
        
        setSlotSequence(slotIndex, sequence, steps);
    }
    
    void setupColorCycle(uint8_t slotIndex, const uint16_t* colors, uint8_t colorCount) {
        if (slotIndex >= TRANSPARENT_SLOT_COUNT || !colors || colorCount == 0) return;
        setSlotSequence(slotIndex, colors, colorCount);
    }
    
    void setupFlashEffect(uint8_t slotIndex, uint16_t color1, uint16_t color2, uint8_t flashRate = 4) {
        if (slotIndex >= TRANSPARENT_SLOT_COUNT || flashRate == 0) return;
        
        uint16_t sequence[8];
        for (uint8_t i = 0; i < flashRate && i < 4; i++) {
            sequence[i * 2] = color1;
            sequence[i * 2 + 1] = color2;
        }
        
        setSlotSequence(slotIndex, sequence, flashRate * 2);
    }
    
private:
    // Get LUT array index for slot
    uint16_t getSlotLUTIndex(uint8_t slotIndex) const {
        switch (slotIndex) {
            case 0: return SLOT_0_INDEX;  // (63, 61)
            case 1: return SLOT_1_INDEX;  // (63, 62)
            case 2: return SLOT_2_INDEX;  // (63, 63)
            case 3: return SLOT_3_INDEX;  // (63, 60)
            default: return 0;
        }
    }
    
    // Scale RGB565 color by intensity factor (0.0 to 1.0)
    uint16_t scaleColor(uint16_t color, float intensity) const {
        if (intensity <= 0.0f) return 0x0000;
        if (intensity >= 1.0f) return color;
        
        // Extract RGB components
        uint8_t r = (color >> 11) & 0x1F;
        uint8_t g = (color >> 5) & 0x3F;
        uint8_t b = color & 0x1F;
        
        // Scale components
        r = (uint8_t)(r * intensity);
        g = (uint8_t)(g * intensity);
        b = (uint8_t)(b * intensity);
        
        // Recombine
        return (r << 11) | (g << 5) | b;
    }
};

// Global enhanced LUT system instance
extern EnhancedLUTSystem enhancedLUT;

// Helper macros for transparency checking
#define IS_PIXEL_TRANSPARENT(lutX, lutY) enhancedLUT.isTransparent(lutX, lutY)
#define LOOKUP_LUT_COLOR(lutX, lutY) enhancedLUT.lookupColor(lutX, lutY)
