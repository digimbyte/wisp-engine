#pragma once

// WISP Asset Type Definitions
// Provides type constants for asset management in the WISP engine

#include <stdint.h>

// Asset type enumeration
enum WispAssetType : uint8_t {
    WISP_ASSET_NONE = 0,
    WISP_ASSET_SPRITE = 1,
    WISP_ASSET_TILEMAP = 2,
    WISP_ASSET_PALETTE = 3,
    WISP_ASSET_SOUND = 4,
    WISP_ASSET_MUSIC = 5,
    WISP_ASSET_SCRIPT = 6,
    WISP_ASSET_DATA = 7,
    WISP_ASSET_FONT = 8,
    WISP_ASSET_TEXTURE = 9,
    WISP_ASSET_SHADER = 10,
    WISP_ASSET_ANIMATION = 11,
    WISP_ASSET_LEVEL = 12,
    WISP_ASSET_CONFIG = 13,
    WISP_ASSET_SAVE = 14,
    WISP_ASSET_MAX = 15
};

// Asset flags
#define WISP_ASSET_FLAG_COMPRESSED  0x01
#define WISP_ASSET_FLAG_ENCRYPTED   0x02
#define WISP_ASSET_FLAG_CACHED      0x04
#define WISP_ASSET_FLAG_PERSISTENT  0x08
#define WISP_ASSET_FLAG_LAZY_LOAD   0x10

// Asset header structure
struct WispAssetHeader {
    uint32_t magic;         // Asset identifier "WASR"
    uint16_t version;       // Asset format version
    WispAssetType type;     // Asset type
    uint8_t flags;          // Asset flags
    uint32_t size;          // Asset data size
    uint32_t checksum;      // CRC32 checksum
    uint32_t timestamp;     // Creation timestamp
    char name[32];          // Asset name
};

// Magic constant for asset headers
#define WISP_ASSET_MAGIC 0x52534157  // "WASR"

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

// Function declarations for asset management
#ifdef __cplusplus
extern "C" {
#endif

const char* wispAssetTypeToString(WispAssetType type);
WispAssetType wispStringToAssetType(const char* str);
bool wispValidateAssetHeader(const WispAssetHeader* header);

#ifdef __cplusplus
}
#endif
