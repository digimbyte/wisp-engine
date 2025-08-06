#pragma once

// WISP Runtime Loader
// Parses WISP bundles at runtime and provides access to embedded assets

#include <stdint.h>
#include <string.h>
#include "../../../src/system/asset_types.h"

// Use existing String class from engine_common.h  
// Forward declaration only - don't redefine
class String;

namespace WispEngine {
namespace App {

// WISP Bundle Header Structure (matches wisp_rom_builder.py)
struct WispBundleHeader {
    uint32_t magic;         // 'WISP' (0x50534957)
    uint32_t version;       // Bundle format version
    uint16_t entryCount;    // Number of asset entries
    uint16_t configSize;    // Embedded YAML config size
    uint32_t reserved;      // Reserved for future use
};

// WISP Asset Entry Structure (48 bytes total)
struct WispAssetEntry {
    char name[32];          // Asset name (null-terminated)
    uint32_t offset;        // Offset from data section start
    uint32_t size;          // Asset data size in bytes
    uint8_t type;           // Asset type (WispAssets::AssetType)
    uint8_t flags;          // Asset flags
    uint8_t reserved[6];    // Reserved for alignment
};

// Asset loading result
enum WispLoadResult {
    WISP_LOAD_SUCCESS = 0,
    WISP_LOAD_FILE_NOT_FOUND,
    WISP_LOAD_INVALID_BUNDLE,
    WISP_LOAD_ASSET_NOT_FOUND,
    WISP_LOAD_MEMORY_ERROR,
    WISP_LOAD_IO_ERROR
};

class WispRuntimeLoader {
private:
    // Bundle data
    uint8_t* bundleData;
    size_t bundleSize;
    bool ownsData;
    
    // Parsed structure
    WispBundleHeader header;
    WispAssetEntry* entries;
    const char* configData;
    uint8_t* assetDataStart;
    
    // Internal helpers
    bool validateBundle();
    WispAssetEntry* findAssetEntry(const String& assetName);
    
public:
    WispRuntimeLoader();
    ~WispRuntimeLoader();
    
    // Bundle loading
    WispLoadResult loadFromFile(const String& filePath);
    WispLoadResult loadFromMemory(const uint8_t* data, size_t size, bool copyData = true);
    void unload();
    
    // Bundle information
    bool isLoaded() const { return bundleData != nullptr; }
    uint16_t getAssetCount() const { return header.entryCount; }
    const char* getConfigData() const { return configData; }
    uint16_t getConfigSize() const { return header.configSize; }
    
    // Asset access
    bool hasAsset(const String& assetName);
    WispLoadResult getAssetInfo(const String& assetName, WispAssetEntry& info);
    const uint8_t* getAssetData(const String& assetName);
    WispLoadResult extractAsset(const String& assetName, uint8_t** data, size_t* size);
    
    // Asset enumeration
    const WispAssetEntry* getAssetEntry(uint16_t index) const;
    
    // Configuration parsing
    String getConfigValue(const String& key);
    
    // Memory management
    void freeExtractedAsset(uint8_t* data);
};

} // namespace App
} // namespace WispEngine
