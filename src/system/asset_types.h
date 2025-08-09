// wisp_asset_types.h
#pragma once
#include <stdint.h>

// Wisp Engine Asset Type Definitions
// See docs/wisp_file_formats.md for complete specification

namespace WispAssets {

// Asset Type IDs (used in WISP bundles)
enum AssetType : uint8_t {
    ASSET_UNKNOWN = 0x00,
    ASSET_PALETTE = 0x01,    // .wlut files (palettes/LUTs)
    ASSET_SPRITE = 0x02,     // .art files (sprite graphics)
    ASSET_TILEMAP = 0x03,    // Tile-based maps
    ASSET_AUDIO = 0x04,      // .sfx files (sound effects)
    ASSET_FONT = 0x05,       // Font data
    ASSET_CONFIG = 0x06,     // JSON configuration
    ASSET_SOURCE = 0x07,     // .ash files (uncompiled C++)
    ASSET_BINARY = 0x08,     // .wash files (compiled code)
    ASSET_LAYOUT = 0x09,     // Scene layout data (game levels/interfaces)
    ASSET_PANEL = 0x0A,      // Panel data within layouts
    ASSET_DEPTH = 0x0B,      // Depth map data for 2.5D per-pixel rendering
    ASSET_LEVEL = 0x0C       // Legacy level data (for backward compatibility)
};

// File Format Magic Numbers
constexpr uint32_t MAGIC_WISP = 0x50534957;  // 'WISP' - Master bundle
constexpr uint32_t MAGIC_WLUT = 0x54554C57;  // 'WLUT' - Palette/LUT
constexpr uint32_t MAGIC_WART = 0x54524157;  // 'WART' - Sprite graphics
constexpr uint32_t MAGIC_WSFX = 0x58465357;  // 'WSFX' - Audio
constexpr uint32_t MAGIC_WASH = 0x48534157;  // 'WASH' - Source code
constexpr uint32_t MAGIC_WBIN = 0x4E494257;  // 'WBIN' - Compiled binary

// Palette/LUT Format Types
enum PaletteFormat : uint32_t {
    LUT_64x64 = 0x4C555436,    // 'LUT6' - 64×64 lookup table
    LUT_32x32 = 0x4C555433,    // 'LUT3' - 32×32 lookup table
    PAL_16 = 0x50414C31,       // 'PAL1' - 16 color palette
    PAL_64 = 0x50414C36,       // 'PAL6' - 64 color palette
    PAL_256 = 0x50414C38       // 'PAL8' - 256 color palette
};

// Memory Profile Recommendations
struct MemoryProfile {
    const char* name;
    AssetType primaryPalette;
    PaletteFormat paletteFormat;
    uint32_t maxMemoryKB;
    uint16_t maxSprites;
    uint8_t maxPalettes;
};

// Predefined memory profiles
constexpr MemoryProfile PROFILE_MINIMAL = {
    "MINIMAL", ASSET_PALETTE, PAL_16, 32, 32, 2
};

constexpr MemoryProfile PROFILE_BALANCED = {
    "BALANCED", ASSET_PALETTE, PAL_64, 128, 128, 4
};

constexpr MemoryProfile PROFILE_FULL = {
    "FULL", ASSET_PALETTE, LUT_64x64, 256, 256, 8
};

// Helper functions
inline const char* getAssetTypeName(AssetType type) {
    switch (type) {
        case ASSET_PALETTE: return "Palette";
        case ASSET_SPRITE: return "Sprite";
        case ASSET_TILEMAP: return "Tilemap";
        case ASSET_AUDIO: return "Audio";
        case ASSET_FONT: return "Font";
        case ASSET_CONFIG: return "Config";
        case ASSET_SOURCE: return "Source";
        case ASSET_BINARY: return "Binary";
        case ASSET_LEVEL: return "Level";
        case ASSET_DEPTH: return "Depth";
        default: return "Unknown";
    }
}

inline const char* getFileExtension(AssetType type) {
    switch (type) {
        case ASSET_PALETTE: return ".wlut";
        case ASSET_SPRITE: return ".art";
        case ASSET_AUDIO: return ".sfx";
        case ASSET_SOURCE: return ".ash";
        case ASSET_BINARY: return ".wash";
        case ASSET_CONFIG: return ".json";
        default: return ".dat";
    }
}

inline uint32_t getFormatMagic(AssetType type) {
    switch (type) {
        case ASSET_PALETTE: return MAGIC_WLUT;
        case ASSET_SPRITE: return MAGIC_WART;
        case ASSET_AUDIO: return MAGIC_WSFX;
        case ASSET_SOURCE: return MAGIC_WASH;
        case ASSET_BINARY: return MAGIC_WBIN;
        default: return 0;
    }
}

// Memory usage calculations
inline uint32_t getPaletteMemoryUsage(PaletteFormat format) {
    switch (format) {
        case PAL_16: return 32;      // 16 colors × 2 bytes
        case PAL_64: return 128;     // 64 colors × 2 bytes
        case PAL_256: return 512;    // 256 colors × 2 bytes
        case LUT_32x32: return 2048; // 1024 colors × 2 bytes
        case LUT_64x64: return 8192; // 4096 colors × 2 bytes
        default: return 0;
    }
}

inline bool isCompatibleWithProfile(AssetType type, PaletteFormat format, const MemoryProfile& profile) {
    if (type == ASSET_PALETTE) {
        uint32_t usage = getPaletteMemoryUsage(format);
        uint32_t maxUsage = getPaletteMemoryUsage(profile.paletteFormat);
        return usage <= maxUsage;
    }
    return true; // Other asset types are always compatible
}

} // namespace WispAssets
