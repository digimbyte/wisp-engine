#pragma once

// WISP Asset Type Definitions - UNIFIED VERSION
// This file now imports the authoritative definitions from asset_types.h
// All asset type enums and constants are now consistent across the engine

#include <stdint.h>
#include "asset_types.h"

// Use the unified asset types from WispAssets namespace
using WispAssetType = WispAssets::AssetType;

// Asset type constants for compatibility (map to unified types)
constexpr WispAssetType WISP_ASSET_NONE = WispAssets::ASSET_UNKNOWN;
constexpr WispAssetType WISP_ASSET_PALETTE = WispAssets::ASSET_PALETTE;  // 0x01
constexpr WispAssetType WISP_ASSET_SPRITE = WispAssets::ASSET_SPRITE;    // 0x02
constexpr WispAssetType WISP_ASSET_TILEMAP = WispAssets::ASSET_TILEMAP;  // 0x03
constexpr WispAssetType WISP_ASSET_SOUND = WispAssets::ASSET_AUDIO;      // 0x04
constexpr WispAssetType WISP_ASSET_FONT = WispAssets::ASSET_FONT;        // 0x05
constexpr WispAssetType WISP_ASSET_CONFIG = WispAssets::ASSET_CONFIG;    // 0x06
constexpr WispAssetType WISP_ASSET_SOURCE = WispAssets::ASSET_SOURCE;    // 0x07
constexpr WispAssetType WISP_ASSET_BINARY = WispAssets::ASSET_BINARY;    // 0x08
constexpr WispAssetType WISP_ASSET_LEVEL = WispAssets::ASSET_LEVEL;      // 0x09
constexpr WispAssetType WISP_ASSET_DEPTH = WispAssets::ASSET_DEPTH;      // 0x0A

// Legacy aliases for backwards compatibility
constexpr WispAssetType WISP_ASSET_MUSIC = WispAssets::ASSET_AUDIO;
constexpr WispAssetType WISP_ASSET_SCRIPT = WispAssets::ASSET_SOURCE;
constexpr WispAssetType WISP_ASSET_DATA = WispAssets::ASSET_CONFIG;
constexpr WispAssetType WISP_ASSET_TEXTURE = WispAssets::ASSET_SPRITE;
constexpr WispAssetType WISP_ASSET_SHADER = WispAssets::ASSET_SOURCE;
constexpr WispAssetType WISP_ASSET_ANIMATION = WispAssets::ASSET_SPRITE;
constexpr WispAssetType WISP_ASSET_SAVE = WispAssets::ASSET_CONFIG;
constexpr WispAssetType WISP_ASSET_MAX = static_cast<WispAssetType>(0x0B);

// Asset flags
#define WISP_ASSET_FLAG_COMPRESSED  0x01
#define WISP_ASSET_FLAG_ENCRYPTED   0x02
#define WISP_ASSET_FLAG_CACHED      0x04
#define WISP_ASSET_FLAG_PERSISTENT  0x08
#define WISP_ASSET_FLAG_LAZY_LOAD   0x10

// Asset header structure
struct WispAssetHeader {
    uint32_t magic;         // Asset identifier - use WispAssets::getFormatMagic()
    uint16_t version;       // Asset format version
    WispAssetType type;     // Asset type
    uint8_t flags;          // Asset flags
    uint32_t size;          // Asset data size
    uint32_t checksum;      // CRC32 checksum
    uint32_t timestamp;     // Creation timestamp
    char name[32];          // Asset name
};

// Magic constant for asset headers (use individual format magics instead)
#define WISP_ASSET_MAGIC 0x52534157  // "WASR" - deprecated, use WispAssets::getFormatMagic()

// Asset loading result codes
enum WispAssetResult : uint8_t {
    WISP_ASSET_SUCCESS = 0,
    WISP_ASSET_ERROR_NOT_FOUND = 1,
    WISP_ASSET_ERROR_CORRUPTED = 2,
    WISP_ASSET_ERROR_UNSUPPORTED = 3,
    WISP_ASSET_ERROR_MEMORY = 4,
    WISP_ASSET_ERROR_IO = 5,
    WISP_ASSET_ERROR_INVALID = 6
};

// Helper functions using unified asset system
inline const char* wispAssetTypeToString(WispAssetType type) {
    return WispAssets::getAssetTypeName(type);
}

inline const char* wispGetFileExtension(WispAssetType type) {
    return WispAssets::getFileExtension(type);
}

inline uint32_t wispGetFormatMagic(WispAssetType type) {
    return WispAssets::getFormatMagic(type);
}

// Function declarations for asset management
#ifdef __cplusplus
extern "C" {
#endif

WispAssetType wispStringToAssetType(const char* str);
bool wispValidateAssetHeader(const WispAssetHeader* header);

#ifdef __cplusplus
}
#endif
