// Magic Channel Animation System
// Updates magic channel colors (0x1000-0x1004) each app frame tick
// Channels cycle through color sequences loaded from WLUT assets
//
// Channel Usage Guidelines:
// - Channel 0 (0x1000): Primary transparency - discouraged to change but allowed for special effects
// - Channels 1-4 (0x1001-0x1004): Intended for color animations (player colors, UI themes, etc.)

#pragma once
#include <stdint.h>
#include <string.h>
#include "../app/wisp_runtime_loader.h"
#include "../../system/asset_types.h"

using namespace WispAssets;

// Magic channel configuration
#define MAGIC_CHANNEL_COUNT 5
#define MAX_WLUT_COLORS 4096  // Max colors in a 64x64 WLUT
#define MAX_SEQUENCE_COLORS 128  // Hard limit for color sequence in memory

// Magic channel animation state
struct MagicChannelState {
    bool enabled;                    // Whether this channel is active
    uint16_t* colorSequence;         // Pointer to color sequence from WLUT
    uint16_t sequenceLength;         // Number of colors in sequence
    uint16_t currentIndex;           // Current position in sequence (auto-increments each frame)
    uint16_t currentColor;           // Current active color for this channel
    char wlutAssetName[64];          // Name of WLUT asset providing colors
    bool hasWLUT;                    // Whether WLUT data is loaded
};

// Magic Channel Animation System
class MagicChannelSystem {
private:
    MagicChannelState channels[MAGIC_CHANNEL_COUNT];
    WispEngine::App::WispRuntimeLoader* assetLoader;
    uint32_t lastFrameTick;
    bool systemEnabled;
    
    // WLUT asset references - no memory duplication, point directly to ROM data
    struct WLUTReference {
        char assetName[64];
        const uint16_t* romColorData;     // Direct pointer to ROM asset data
        uint16_t colorCount;              // Actual count from asset
        uint16_t effectiveCount;          // Count used for cycling (cropped to 128 max)
        bool valid;
    } wlutRefs[MAGIC_CHANNEL_COUNT];
    
public:
    MagicChannelSystem() {
        assetLoader = nullptr;
        lastFrameTick = 0;
        systemEnabled = true;
        
        // Initialize all channels as disabled
        for (int i = 0; i < MAGIC_CHANNEL_COUNT; i++) {
            channels[i].enabled = false;
            channels[i].colorSequence = nullptr;
            channels[i].sequenceLength = 0;
            channels[i].currentIndex = 0;
            channels[i].currentColor = 0x1000 + i; // Default magic color
            channels[i].wlutAssetName[0] = '\0';
            channels[i].hasWLUT = false;
            
            // Initialize WLUT references
            wlutRefs[i].assetName[0] = '\0';
            wlutRefs[i].romColorData = nullptr;
            wlutRefs[i].colorCount = 0;
            wlutRefs[i].effectiveCount = 0;
            wlutRefs[i].valid = false;
        }
    }
    
    ~MagicChannelSystem() {
        // No memory management needed - we only use ROM pointers!
        // The WispRuntimeLoader manages the ROM lifecycle
        clearAllChannels();
    }
    
    // Set asset loader reference
    void setAssetLoader(WispEngine::App::WispRuntimeLoader* loader) {
        assetLoader = loader;
    }
    
    // Configure a magic channel to use colors from a WLUT asset
    bool setupChannelFromWLUT(uint8_t channelNumber, const char* wlutAssetName) {
        if (channelNumber >= MAGIC_CHANNEL_COUNT || !wlutAssetName || !assetLoader) {
            return false;
        }
        
        // Check if asset exists in bundle
        if (!assetLoader->hasAsset(String(wlutAssetName))) {
            ESP_LOGE("MagicChannels", "WLUT asset '%s' not found in bundle", wlutAssetName);
            return false;
        }
        
        // Load WLUT asset data
        if (!loadWLUTAsset(channelNumber, wlutAssetName)) {
            return false;
        }
        
        MagicChannelState& channel = channels[channelNumber];
        strncpy(channel.wlutAssetName, wlutAssetName, sizeof(channel.wlutAssetName) - 1);
        channel.wlutAssetName[sizeof(channel.wlutAssetName) - 1] = '\0';
        channel.enabled = true;
        channel.hasWLUT = true;
        channel.currentIndex = 0;
        
        ESP_LOGI("MagicChannels", "Channel %d configured with WLUT '%s' (%d colors)", 
                 channelNumber, wlutAssetName, channel.sequenceLength);
        
        return true;
    }
    
    // Disable a magic channel (revert to transparent)
    void disableChannel(uint8_t channelNumber) {
        if (channelNumber >= MAGIC_CHANNEL_COUNT) return;
        
        MagicChannelState& channel = channels[channelNumber];
        channel.enabled = false;
        channel.hasWLUT = false;
        channel.currentColor = 0x1000 + channelNumber; // Revert to default magic color (transparent)
        
        ESP_LOGI("MagicChannels", "Channel %d disabled (transparent)", channelNumber);
    }
    
    // Clear a magic channel (reset to default magic number - keeps system enabled)
    void clearChannel(uint8_t channelNumber) {
        if (channelNumber >= MAGIC_CHANNEL_COUNT) return;
        
        MagicChannelState& channel = channels[channelNumber];
        // Reset color to default magic number, but keep channel potentially available
        channel.currentColor = 0x1000 + channelNumber; // Reset to default magic color (transparent)
        
        // If channel was enabled with WLUT, reset index to start
        if (channel.enabled && channel.hasWLUT) {
            channel.currentIndex = 0;
        }
        
        ESP_LOGI("MagicChannels", "Channel %d cleared (reset to magic number 0x%04X)", 
                 channelNumber, channel.currentColor);
    }
    
    // Update all channels for current app frame tick
    void updateChannelsForFrame(uint32_t currentFrameTick) {
        if (!systemEnabled || currentFrameTick == lastFrameTick) {
            return; // No update needed
        }
        
        lastFrameTick = currentFrameTick;
        
        // Update each enabled channel
        for (uint8_t i = 0; i < MAGIC_CHANNEL_COUNT; i++) {
            MagicChannelState& channel = channels[i];
            
            if (!channel.enabled || !channel.hasWLUT || channel.sequenceLength == 0) {
                continue;
            }
            
            // Advance to next color in sequence with safe bounds checking
            channel.currentIndex = safeAdvanceIndex(channel.currentIndex, channel.sequenceLength);
            
            // Bounds-checked color access
            if (channel.currentIndex < channel.sequenceLength && channel.colorSequence) {
                channel.currentColor = channel.colorSequence[channel.currentIndex];
            } else {
                // Fallback to default magic color on bounds error
                channel.currentColor = 0x1000 + i;
                channel.currentIndex = 0; // Reset to safe index
                ESP_LOGW("MagicChannels", "Index bounds error on channel %d, reset to default", i);
            }
        }
    }
    
    // Get current color for a magic channel
    uint16_t getChannelColor(uint8_t channelNumber) const {
        if (channelNumber >= MAGIC_CHANNEL_COUNT) {
            return 0x1000; // Default transparent
        }
        
        return channels[channelNumber].currentColor;
    }
    
    // Get current colors for all channels (for LUT updating)
    void getCurrentChannelColors(uint16_t* colors) {
        for (uint8_t i = 0; i < MAGIC_CHANNEL_COUNT; i++) {
            colors[i] = getChannelColor(i);
        }
    }
    
    // Check if magic color should be resolved to current channel color
    uint16_t resolveMagicColor(uint16_t color) const {
        if (isMagicColor(color)) {
            uint8_t channel = getMagicChannel(color);
            if (channel < MAGIC_CHANNEL_COUNT) {
                return getChannelColor(channel);
            }
        }
        return color; // Not a magic color, return unchanged
    }
    
    // System enable/disable
    void setEnabled(bool enabled) { systemEnabled = enabled; }
    bool isEnabled() const { return systemEnabled; }
    
    // Clear all channels (used by destructor)
    void clearAllChannels() {
        for (uint8_t i = 0; i < MAGIC_CHANNEL_COUNT; i++) {
            clearChannel(i);
            wlutRefs[i].valid = false;
            wlutRefs[i].romColorData = nullptr;
        }
    }
    
    // App lifecycle management - called when ROM changes
    void onROMUnloaded() {
        ESP_LOGI("MagicChannels", "ROM unloaded - clearing all channel references");
        clearAllChannels();
    }
    
    // Debug status
    void printChannelStatus() const {
        ESP_LOGI("MagicChannels", "Magic Channel Status:");
        for (uint8_t i = 0; i < MAGIC_CHANNEL_COUNT; i++) {
            const MagicChannelState& channel = channels[i];
            if (channel.enabled) {
                ESP_LOGI("MagicChannels", "  Channel %d: Active, WLUT='%s', %d colors, current=0x%04X", 
                         i, channel.wlutAssetName, channel.sequenceLength, channel.currentColor);
            } else {
                ESP_LOGI("MagicChannels", "  Channel %d: Disabled (transparent)", i);
            }
        }
    }
    
private:
    // Helper function to find largest power of two less than or equal to value
    uint16_t largestPowerOfTwo(uint16_t value) const {
        if (value == 0) return 1;
        
        // Find the highest bit set
        uint16_t result = 1;
        while (result <= value >> 1) {
            result <<= 1;
        }
        
        return result;
    }
    
    // Safe index advancement with bounds correction
    uint16_t safeAdvanceIndex(uint16_t currentIndex, uint16_t sequenceLength) const {
        if (sequenceLength == 0) return 0;
        
        // Simple safe increment with wraparound
        uint16_t nextIndex = (currentIndex + 1) % sequenceLength;
        
        // Additional safety check - if somehow we're still out of bounds, clamp to 0
        if (nextIndex >= sequenceLength) {
            return 0;
        }
        
        return nextIndex;
    }
    
    // Load WLUT asset using direct ROM pointers - zero memory duplication!
    bool loadWLUTAsset(uint8_t channelNumber, const char* assetName) {
        if (channelNumber >= MAGIC_CHANNEL_COUNT || !assetName || !assetLoader) {
            return false;
        }
        
        WLUTReference& ref = wlutRefs[channelNumber];
        
        // Check if already referencing the same asset
        if (ref.valid && strcmp(ref.assetName, assetName) == 0) {
            // Already loaded this asset - reuse ROM pointer
            channels[channelNumber].colorSequence = const_cast<uint16_t*>(ref.romColorData);
            channels[channelNumber].sequenceLength = ref.effectiveCount;
            return true;
        }
        
        // Get asset info
        WispEngine::App::WispAssetEntry assetInfo;
        if (assetLoader->getAssetInfo(String(assetName), assetInfo) != WispEngine::App::WISP_LOAD_SUCCESS) {
            ESP_LOGE("MagicChannels", "Failed to get asset info for '%s'", assetName);
            return false;
        }
        
        // Verify it's a palette asset
        if (assetInfo.type != ASSET_PALETTE) {
            ESP_LOGE("MagicChannels", "Asset '%s' is not a palette (type: %d)", assetName, assetInfo.type);
            return false;
        }
        
        // Get direct ROM pointer to asset data - NO COPYING!
        const uint8_t* rawData = assetLoader->getAssetData(String(assetName));
        if (!rawData) {
            ESP_LOGE("MagicChannels", "Failed to get ROM asset pointer for '%s'", assetName);
            return false;
        }
        
        // Parse WLUT header (magic, format, width, height, colorCount, reserved)
        if (assetInfo.size < 16) { // Minimum header size
            ESP_LOGE("MagicChannels", "WLUT asset '%s' too small", assetName);
            return false;
        }
        
        const uint8_t* dataPtr = rawData;
        
        // Skip magic (4 bytes) and format (4 bytes)
        dataPtr += 8;
        
        // Skip width and height (4 bytes total)
        dataPtr += 4;
        
        // Read color count (2 bytes)
        uint16_t colorCount = *((uint16_t*)dataPtr);
        dataPtr += 2;
        
        // Skip reserved (2 bytes)
        dataPtr += 2;
        
        // Calculate effective count (cropped to 128 max for cycling)
        uint16_t effectiveCount = colorCount;
        if (colorCount > MAX_SEQUENCE_COLORS) {
            ESP_LOGW("MagicChannels", "WLUT asset '%s' has %d colors, cropping cycle to %d", 
                     assetName, colorCount, MAX_SEQUENCE_COLORS);
            effectiveCount = MAX_SEQUENCE_COLORS;
        }
        
        // Verify we have enough data for the effective colors
        uint32_t expectedDataSize = 16 + (effectiveCount * 2);
        if (assetInfo.size < expectedDataSize) {
            ESP_LOGE("MagicChannels", "WLUT asset '%s' data size mismatch", assetName);
            return false;
        }
        
        // Store direct ROM pointer - NO MEMORY ALLOCATION!
        ref.romColorData = (const uint16_t*)dataPtr;
        ref.colorCount = colorCount;
        ref.effectiveCount = effectiveCount;
        strncpy(ref.assetName, assetName, sizeof(ref.assetName) - 1);
        ref.assetName[sizeof(ref.assetName) - 1] = '\0';
        ref.valid = true;
        
        // Update channel to point directly to ROM data
        channels[channelNumber].colorSequence = const_cast<uint16_t*>(ref.romColorData);
        channels[channelNumber].sequenceLength = ref.effectiveCount;
        
        ESP_LOGI("MagicChannels", "Channel %d now points to ROM WLUT '%s' (%d colors, cycling %d)", 
                 channelNumber, assetName, colorCount, effectiveCount);
        
        return true;
    }
};

// Global magic channel system instance
extern MagicChannelSystem magicChannels;
