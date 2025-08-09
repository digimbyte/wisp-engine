#pragma once

// WISP ROM Segmented Loader
// Efficient lazy loading for WISP ROMs respecting the actual architecture:
// - Config/metadata loaded immediately (small)
// - Asset table loaded immediately (for fast lookup)  
// - Assets loaded on-demand with LRU cache
// - Database entries streamed as needed
// - App logic/panels loaded when accessed

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "engine_common.h"
#include "wisp_runtime_loader.h"

namespace WispEngine {
namespace App {

// WISP ROM loading strategies
enum WispLoadStrategy {
    WISP_LOAD_IMMEDIATE,     // Load immediately and keep in memory
    WISP_LOAD_ON_DEMAND,     // Load when first accessed
    WISP_LOAD_STREAM,        // Stream in segments
    WISP_LOAD_CACHED         // Load on demand with LRU cache
};

// Asset categories for different loading strategies
enum WispAssetCategory {
    WISP_CATEGORY_CONFIG,     // App metadata, requirements - always immediate
    WISP_CATEGORY_LOGIC,      // Compiled binaries (.wash) - immediate
    WISP_CATEGORY_UI,         // UI panel data - on demand
    WISP_CATEGORY_GRAPHICS,   // Sprites, palettes - cached
    WISP_CATEGORY_AUDIO,      // Sound effects, music - stream
    WISP_CATEGORY_DATA,       // Levels, database - on demand
    WISP_CATEGORY_SOURCE      // Source code (.ash) - rarely loaded
};

// Cache slot for asset segments
struct WispAssetCache {
    uint16_t assetId;
    uint32_t segmentOffset;
    uint8_t* data;
    uint32_t size;
    uint32_t lastAccessed;
    WispLoadStrategy strategy;
    bool dirty;
    
    WispAssetCache() : assetId(0), segmentOffset(0), data(nullptr), 
                       size(0), lastAccessed(0), strategy(WISP_LOAD_ON_DEMAND), dirty(false) {}
    
    ~WispAssetCache() { 
        if (data) free(data); 
    }
};

// ROM section info for efficient access
struct WispROMSection {
    uint32_t offset;
    uint32_t size;
    bool loaded;
    void* data;
    
    WispROMSection() : offset(0), size(0), loaded(false), data(nullptr) {}
};

class WispSegmentedLoader {
private:
    // File handle for streaming
    FILE* romFile;
    String romPath;
    
    // ROM structure (loaded immediately)
    WispBundleHeader header;
    WispAssetEntry* assetTable;    // Full asset table for fast lookup
    char* configData;              // App configuration (YAML)
    
    // ROM sections
    WispROMSection sections[WISP_CATEGORY_SOURCE + 1];
    
    // Asset cache system
    static const uint8_t MAX_CACHED_ASSETS = 16;
    WispAssetCache assetCache[MAX_CACHED_ASSETS];
    uint8_t cacheSize;
    
    // Memory management
    uint32_t maxCacheMemory;       // Maximum memory for cached assets
    uint32_t currentCacheMemory;   // Current cache memory usage
    
    // Performance tracking
    uint32_t cacheHits;
    uint32_t cacheMisses;
    uint32_t streamReads;
    
public:
    WispSegmentedLoader();
    ~WispSegmentedLoader();
    
    // === ROM MANAGEMENT ===
    
    WispLoadResult openROM(const String& filePath);
    void closeROM();
    bool isOpen() const { return romFile != nullptr; }
    
    // Load essential ROM components (config + asset table)
    WispLoadResult loadROMStructure();
    
    // === CONFIGURATION ACCESS (Always Available) ===
    
    String getConfigValue(const String& key);
    const char* getConfigData() const { return configData; }
    uint16_t getConfigSize() const { return header.configSize; }
    
    // App metadata from config
    String getAppName();
    String getAppVersion(); 
    String getAppAuthor();
    String getAppDescription();
    
    // === ASSET ACCESS (Respects Loading Strategies) ===
    
    // Check asset existence (lightweight - uses asset table)
    bool hasAsset(const String& assetName);
    bool hasAssetOfType(WispAssetType type);
    
    // Get asset info without loading data
    WispLoadResult getAssetInfo(const String& assetName, WispAssetEntry& info);
    
    // Load asset data according to its category strategy
    const uint8_t* getAssetData(const String& assetName);
    WispLoadResult loadAsset(const String& assetName, uint8_t** data, size_t* size);
    
    // Stream large assets in chunks
    uint16_t openAssetStream(const String& assetName);
    WispLoadResult readAssetStream(uint16_t streamId, uint8_t* buffer, uint32_t size, uint32_t* bytesRead);
    void closeAssetStream(uint16_t streamId);
    
    // === CATEGORY-SPECIFIC ACCESS ===
    
    // Logic assets (.wash binaries) - loaded immediately
    const uint8_t* getMainBinary();
    const uint8_t* getLibraryBinary(const String& libName);
    
    // UI Panel assets - loaded on demand
    const uint8_t* getUIPanel(const String& panelName);
    bool hasUIPanel(const String& panelName);
    
    // Graphics assets - cached with LRU
    const uint8_t* getSprite(const String& spriteName);
    const uint8_t* getPalette(const String& paletteName);
    void preloadGraphicsAssets(const String* assetNames, uint8_t count);
    
    // Audio assets - streamed
    uint16_t streamAudio(const String& audioName);
    void preloadCriticalAudio(const String* audioNames, uint8_t count);
    
    // Layout/Scene assets - loaded on demand (game levels/interfaces)
    const uint8_t* getLayoutData(const String& layoutName);
    const uint8_t* getPanelData(const String& panelName);
    const uint8_t* getDatabaseSection(const String& sectionName);
    
    // Layout management - streaming and caching for large scenes
    bool preloadLayout(const String& layoutName);
    void unloadLayout(const String& layoutName);
    bool isLayoutLoaded(const String& layoutName);
    
    // === MEMORY MANAGEMENT ===
    
    void setCacheLimit(uint32_t maxBytes);
    void evictLRUAssets();
    void clearCache();
    void clearCacheCategory(WispAssetCategory category);
    
    // === ROM VALIDATION ===
    
    bool validateROM();
    bool checkAssetIntegrity(const String& assetName);
    uint32_t calculateROMChecksum();
    
    // === PERFORMANCE MONITORING ===
    
    void printCacheStats();
    void printMemoryUsage();
    float getCacheHitRatio() const;
    uint32_t getTotalMemoryUsage() const;
    
private:
    // === INTERNAL ASSET MANAGEMENT ===
    
    WispAssetEntry* findAssetEntry(const String& assetName);
    WispAssetCategory categorizeAsset(const WispAssetEntry* entry);
    WispLoadStrategy getLoadStrategy(WispAssetCategory category);
    
    // Cache management
    WispAssetCache* findCacheSlot(uint16_t assetId);
    WispAssetCache* allocateCacheSlot();
    void evictCacheSlot(WispAssetCache* slot);
    void updateCacheAccess(WispAssetCache* slot);
    
    // File I/O
    WispLoadResult readROMData(uint32_t offset, uint8_t* buffer, uint32_t size);
    WispLoadResult loadAssetToCache(const WispAssetEntry* entry);
    WispLoadResult loadSectionData(WispAssetCategory category);
    
    // Asset ID generation
    uint16_t getAssetId(const String& assetName);
    
    // Memory calculations
    uint32_t calculateAssetMemoryUsage(const WispAssetEntry* entry);
    bool canFitInCache(uint32_t requiredBytes);
};

// === INLINE IMPLEMENTATIONS FOR PERFORMANCE ===

inline bool WispSegmentedLoader::hasAsset(const String& assetName) {
    // Fast lookup using loaded asset table
    return findAssetEntry(assetName) != nullptr;
}

inline WispAssetCategory WispSegmentedLoader::categorizeAsset(const WispAssetEntry* entry) {
    switch (entry->type) {
        case WISP_ASSET_CONFIG:
            return WISP_CATEGORY_CONFIG;
        case WISP_ASSET_BINARY:
            return WISP_CATEGORY_LOGIC;
        case WISP_ASSET_SPRITE:
        case WISP_ASSET_PALETTE:
            return WISP_CATEGORY_GRAPHICS;
        case WISP_ASSET_SOUND:
            return WISP_CATEGORY_AUDIO;
        case WISP_ASSET_LEVEL:
            return WISP_CATEGORY_DATA;
        case WISP_ASSET_SOURCE:
            return WISP_CATEGORY_SOURCE;
        default:
            return WISP_CATEGORY_DATA;
    }
}

inline WispLoadStrategy WispSegmentedLoader::getLoadStrategy(WispAssetCategory category) {
    switch (category) {
        case WISP_CATEGORY_CONFIG:
        case WISP_CATEGORY_LOGIC:
            return WISP_LOAD_IMMEDIATE;
        case WISP_CATEGORY_GRAPHICS:
            return WISP_LOAD_CACHED;
        case WISP_CATEGORY_AUDIO:
            return WISP_LOAD_STREAM;
        case WISP_CATEGORY_UI:
        case WISP_CATEGORY_DATA:
            return WISP_LOAD_ON_DEMAND;
        case WISP_CATEGORY_SOURCE:
            return WISP_LOAD_ON_DEMAND;  // Rarely accessed
        default:
            return WISP_LOAD_ON_DEMAND;
    }
}

inline const uint8_t* WispSegmentedLoader::getAssetData(const String& assetName) {
    WispAssetEntry* entry = findAssetEntry(assetName);
    if (!entry) return nullptr;
    
    WispAssetCategory category = categorizeAsset(entry);
    WispLoadStrategy strategy = getLoadStrategy(category);
    
    switch (strategy) {
        case WISP_LOAD_IMMEDIATE:
            // Should already be loaded in section data
            if (!sections[category].loaded) {
                loadSectionData(category);
            }
            return (uint8_t*)sections[category].data + 
                   (entry->offset - sections[category].offset);
            
        case WISP_LOAD_CACHED:
            // Check cache first
            {
                uint16_t assetId = getAssetId(assetName);
                WispAssetCache* slot = findCacheSlot(assetId);
                if (slot) {
                    updateCacheAccess(slot);
                    cacheHits++;
                    return slot->data;
                }
                
                // Cache miss - load to cache
                cacheMisses++;
                if (loadAssetToCache(entry) == WISP_LOAD_SUCCESS) {
                    slot = findCacheSlot(assetId);
                    return slot ? slot->data : nullptr;
                }
                return nullptr;
            }
            
        case WISP_LOAD_ON_DEMAND:
            // Load directly (not cached)
            // For this simple case, we'll use a temporary load
            // In practice, you might want a small temporary cache
            {
                static uint8_t* tempBuffer = nullptr;
                static uint32_t tempSize = 0;
                
                if (tempSize < entry->size) {
                    free(tempBuffer);
                    tempBuffer = (uint8_t*)malloc(entry->size);
                    tempSize = entry->size;
                }
                
                if (readROMData(entry->offset, tempBuffer, entry->size) == WISP_LOAD_SUCCESS) {
                    return tempBuffer;
                }
                return nullptr;
            }
            
        case WISP_LOAD_STREAM:
            // Streaming assets shouldn't use this method
            // Use openAssetStream() instead
            return nullptr;
    }
    
    return nullptr;
}

} // namespace App  
} // namespace WispEngine
