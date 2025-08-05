// engine/lazy_resource_manager.h - ESP32-C6/S3 Resource Manager using ESP-IDF
// Efficient lazy loading resource management for ESP32 microcontroller
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include "../core/debug.h"  // For debug macros

// Fixed array limits for ESP-IDF compatibility
#define MAX_RESOURCES 256
#define MAX_LEVEL_CHUNKS 64
#define MAX_LOADED_RESOURCES 32
#define MAX_LOADED_CHUNKS 16
#define MAX_CHUNK_SPRITES 32
#define MAX_CHUNK_AUDIO 16
#define MAX_CHUNK_PALETTES 8
#define MAX_CHUNK_ENTITIES 64

// Lazy-loaded resource management system
// Only loads what's needed, unloads when not needed

// Resource types that can be lazy-loaded
enum ResourceType {
    RESOURCE_SPRITE,
    RESOURCE_AUDIO,
    RESOURCE_LEVEL_DATA,
    RESOURCE_FONT,
    RESOURCE_PALETTE
};

// Resource state tracking
enum ResourceState {
    RESOURCE_UNLOADED,      // Not in memory
    RESOURCE_LOADING,       // Currently being loaded
    RESOURCE_LOADED,        // In memory and ready
    RESOURCE_ERROR          // Failed to load
};

// Resource metadata - stored permanently (small footprint)
struct ResourceInfo {
    uint16_t resourceId;
    ResourceType type;
    char filePath[256];     // Where to load from
    uint32_t fileOffset;    // Offset in file
    uint32_t fileSize;      // Size in bytes
    uint32_t memorySize;    // Size when loaded in memory
    uint8_t priority;       // 0=critical, 255=optional
    uint32_t lastAccessed;  // For LRU eviction
    ResourceState state;
    void* data;             // Pointer to loaded data (null if unloaded)
    
    ResourceInfo() : resourceId(0), type(RESOURCE_SPRITE), fileOffset(0), 
                     fileSize(0), memorySize(0), priority(128), 
                     lastAccessed(0), state(RESOURCE_UNLOADED), data(nullptr) {
        filePath[0] = '\0';
    }
};

// Level chunk - contains only what's needed for current area
struct LevelChunk {
    uint16_t chunkId;
    int16_t worldX, worldY;     // World coordinates this chunk covers
    uint16_t width, height;     // Chunk dimensions
    
    // Resources needed for this chunk
    uint16_t requiredSprites[MAX_CHUNK_SPRITES];
    uint16_t requiredAudio[MAX_CHUNK_AUDIO];
    uint16_t requiredPalettes[MAX_CHUNK_PALETTES];
    uint8_t numRequiredSprites;
    uint8_t numRequiredAudio; 
    uint8_t numRequiredPalettes;
    
    // Level-specific data
    uint8_t* tileData;          // Tile map data
    uint8_t* collisionData;     // Collision map
    uint8_t* triggerData;       // Trigger zones
    
    // Entities in this chunk
    struct ChunkEntity {
        uint16_t entityType;
        int16_t x, y;
        uint16_t spriteId;
        uint8_t behavior;       // AI/behavior type
        uint32_t properties;    // Entity-specific data
    };
    ChunkEntity entities[MAX_CHUNK_ENTITIES];
    uint8_t numEntities;
    
    bool loaded;
    uint32_t lastAccessed;
    
    LevelChunk() : chunkId(0), worldX(0), worldY(0), width(0), height(0),
                   tileData(nullptr), collisionData(nullptr), triggerData(nullptr),
                   loaded(false), lastAccessed(0) {}
};

// Lazy resource manager - only loads what's visible/needed
class LazyResourceManager {
private:
    // Resource registry - metadata for all possible resources
    ResourceInfo resourceRegistry[MAX_RESOURCES];
    uint16_t numResources;
    
    // Currently loaded resources
    uint16_t loadedResources[MAX_LOADED_RESOURCES];
    uint8_t numLoadedResources;
    
    // Level chunk system
    LevelChunk levelChunks[MAX_LEVEL_CHUNKS];
    uint16_t numLevelChunks;
    uint16_t loadedChunks[MAX_LOADED_CHUNKS];
    uint8_t numLoadedChunks;
    
    // Memory management
    uint32_t maxMemoryUsage;    // Total memory budget
    uint32_t currentMemoryUsage;
    uint32_t memoryPressureThreshold;
    
    // Current player position for proximity loading
    int16_t playerX, playerY;
    uint16_t loadRadius;        // How far ahead to load
    
    // Performance tracking
    uint32_t loadTime;
    uint32_t unloadTime;
    uint32_t memoryFragmentation;
    
public:
    LazyResourceManager() : 
        numResources(0),
        numLoadedResources(0),
        numLevelChunks(0),
        numLoadedChunks(0),
        maxMemoryUsage(128 * 1024),      // 128KB budget
        currentMemoryUsage(0),
        memoryPressureThreshold(96 * 1024), // Start unloading at 96KB
        playerX(0), playerY(0),
        loadRadius(32),                   // Load chunks within 32 pixels
        loadTime(0), unloadTime(0), memoryFragmentation(0) {}
    
    // Resource registry management
    bool registerResource(uint16_t resourceId, ResourceType type, 
                         const char* filePath, uint32_t offset, uint32_t size);
    
    // Lazy loading interface
    void* getResource(uint16_t resourceId);
    bool isResourceLoaded(uint16_t resourceId);
    void preloadResource(uint16_t resourceId, uint8_t priority = 128);
    void unloadResource(uint16_t resourceId);
    
    // Level chunk management
    bool registerLevelChunk(uint16_t chunkId, int16_t worldX, int16_t worldY,
                           uint16_t width, uint16_t height);
    bool loadChunk(uint16_t chunkId);
    void unloadChunk(uint16_t chunkId);
    LevelChunk* getChunk(uint16_t chunkId);
    
    // Player position tracking for proximity loading
    void updatePlayerPosition(int16_t x, int16_t y);
    void updateProximityLoading();
    
    // Memory management
    void setMemoryBudget(uint32_t maxBytes);
    bool enforceMemoryBudget();
    void garbageCollect();
    uint32_t getCurrentMemoryUsage() const { return currentMemoryUsage; }
    uint32_t getMaxMemoryUsage() const { return maxMemoryUsage; }
    float getMemoryPressure() const;
    
    // Resource streaming for large files
    bool startResourceStream(uint16_t resourceId);
    bool streamResourceChunk(uint16_t resourceId, uint8_t* buffer, uint32_t size);
    void endResourceStream(uint16_t resourceId);
    
    // Debug and monitoring
    void printMemoryStatus();
    void printResourceStatus();
    void printChunkStatus();
    uint16_t* getLoadedResources() { return loadedResources; }
    uint8_t getNumLoadedResources() const { return numLoadedResources; }
    
private:
    // Internal loading/unloading
    bool loadResourceFromFile(ResourceInfo& info);
    void unloadResourceFromMemory(ResourceInfo& info);
    
    // Memory pressure handling
    bool freeMemoryForResource(uint32_t requiredBytes);
    uint16_t findLeastRecentlyUsedResource();
    
    // Chunk proximity calculations
    bool isChunkInRange(const LevelChunk& chunk, int16_t centerX, int16_t centerY, uint16_t radius);
    uint16_t* getChunksInRange(int16_t centerX, int16_t centerY, uint16_t radius, uint8_t* numChunks);
    
    // File I/O helpers
    bool loadDataFromFile(const char* filePath, uint32_t offset, uint32_t size, void* buffer);
    
    // Array search helpers
    ResourceInfo* findResource(uint16_t resourceId);
    LevelChunk* findChunk(uint16_t chunkId);
    bool addLoadedResource(uint16_t resourceId);
    bool removeLoadedResource(uint16_t resourceId);
};

// Implementation of key methods
inline void* LazyResourceManager::getResource(uint16_t resourceId) {
    ResourceInfo* info = findResource(resourceId);
    if (!info) {
        return nullptr;
    }
    
    // Update access time
    info->lastAccessed = esp_timer_get_time() / 1000; // ESP-IDF time in ms
    
    // If already loaded, return data
    if (info->state == RESOURCE_LOADED && info->data) {
        return info->data;
    }
    
    // If not loaded, load it now
    if (info->state == RESOURCE_UNLOADED) {
        if (loadResourceFromFile(*info)) {
            return info->data;
        }
    }
    
    return nullptr;
}

inline bool LazyResourceManager::loadResourceFromFile(ResourceInfo& info) {
    // Check if we have enough memory
    if (currentMemoryUsage + info.memorySize > maxMemoryUsage) {
        if (!freeMemoryForResource(info.memorySize)) {
            WISP_DEBUG_ERROR("RESOURCE", "Cannot free memory for resource");
            return false;
        }
    }
    
    info.state = RESOURCE_LOADING;
    
    // Allocate memory
    info.data = malloc(info.memorySize);
    if (!info.data) {
        WISP_DEBUG_ERROR("RESOURCE", "Memory allocation failed for resource");
        info.state = RESOURCE_ERROR;
        return false;
    }
    
    // Load from file
    uint32_t startTime = esp_timer_get_time();
    bool success = loadDataFromFile(info.filePath, info.fileOffset, info.fileSize, info.data);
    loadTime += esp_timer_get_time() - startTime;
    
    if (success) {
        info.state = RESOURCE_LOADED;
        currentMemoryUsage += info.memorySize;
        addLoadedResource(info.resourceId);
        
        WISP_DEBUG_INFO("RESOURCE", "Loaded resource");
        
        return true;
    } else {
        free(info.data);
        info.data = nullptr;
        info.state = RESOURCE_ERROR;
        
        WISP_DEBUG_ERROR("RESOURCE", "Failed to load resource");
        return false;
    }
}

inline void LazyResourceManager::unloadResourceFromMemory(ResourceInfo& info) {
    if (info.data) {
        uint32_t startTime = esp_timer_get_time();
        
        free(info.data);
        info.data = nullptr;
        info.state = RESOURCE_UNLOADED;
        currentMemoryUsage -= info.memorySize;
        
        // Remove from loaded list
        removeLoadedResource(info.resourceId);
        
        unloadTime += esp_timer_get_time() - startTime;
        
        WISP_DEBUG_INFO("RESOURCE", "Unloaded resource");
    }
}

inline void LazyResourceManager::updatePlayerPosition(int16_t x, int16_t y) {
    playerX = x;
    playerY = y;
    
    // Trigger proximity loading update
    updateProximityLoading();
}

inline void LazyResourceManager::updateProximityLoading() {
    // Get chunks that should be loaded based on player position
    uint8_t numInRange;
    uint16_t* chunksInRange = getChunksInRange(playerX, playerY, loadRadius, &numInRange);
    
    // Load chunks that aren't loaded
    for (uint8_t i = 0; i < numInRange; i++) {
        uint16_t chunkId = chunksInRange[i];
        bool found = false;
        for (uint8_t j = 0; j < numLoadedChunks; j++) {
            if (loadedChunks[j] == chunkId) {
                found = true;
                break;
            }
        }
        if (!found) {
            loadChunk(chunkId);
        }
    }
    
    // Unload chunks that are too far away
    for (uint8_t i = 0; i < numLoadedChunks; i++) {
        uint16_t chunkId = loadedChunks[i];
        LevelChunk* chunk = findChunk(chunkId);
        
        if (chunk) {
            if (!isChunkInRange(*chunk, playerX, playerY, loadRadius * 1.5f)) {
                unloadChunk(chunkId);
                // Remove from loaded list - shift array
                for (uint8_t j = i; j < numLoadedChunks - 1; j++) {
                    loadedChunks[j] = loadedChunks[j + 1];
                }
                numLoadedChunks--;
                i--; // Adjust index after removal
            }
        }
    }
    
    // Enforce memory budget if needed
    if (currentMemoryUsage > memoryPressureThreshold) {
        enforceMemoryBudget();
    }
}

inline bool LazyResourceManager::freeMemoryForResource(uint32_t requiredBytes) {
    uint32_t freedBytes = 0;
    
    // Try to free memory by unloading least recently used resources
    while (freedBytes < requiredBytes && numLoadedResources > 0) {
        uint16_t lruResourceId = findLeastRecentlyUsedResource();
        if (lruResourceId == 0xFFFF) break; // No more resources to unload
        
        ResourceInfo* info = findResource(lruResourceId);
        if (info) {
            freedBytes += info->memorySize;
            unloadResourceFromMemory(*info);
        }
    }
    
    return freedBytes >= requiredBytes;
}

inline void LazyResourceManager::printMemoryStatus() {
    WISP_DEBUG_INFO("RESOURCE", "=== Lazy Resource Manager Status ===");
    WISP_DEBUG_INFO("RESOURCE", "Memory Usage and Statistics");
    WISP_DEBUG_INFO("RESOURCE", "Loaded Resources Count");
    WISP_DEBUG_INFO("RESOURCE", "Loaded Chunks Count");
    WISP_DEBUG_INFO("RESOURCE", "Player Position and Load Radius");
    WISP_DEBUG_INFO("RESOURCE", "====================================");
}
