// engine/lazy_resource_manager.h - ESP32-C6/S3 Resource Manager using ESP-IDF
// Memory-efficient lazy loading resource management for ESP32 microcontroller
#pragma once
#include "../../system/esp32_common.h"  // Pure ESP-IDF native headers
#include <map>
#include <vector>

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
    String filePath;        // Where to load from
    uint32_t fileOffset;    // Offset in file
    uint32_t fileSize;      // Size in bytes
    uint32_t memorySize;    // Size when loaded in memory
    uint8_t priority;       // 0=critical, 255=optional
    uint32_t lastAccessed;  // For LRU eviction
    ResourceState state;
    void* data;             // Pointer to loaded data (null if unloaded)
    
    ResourceInfo() : resourceId(0), type(RESOURCE_SPRITE), fileOffset(0), 
                     fileSize(0), memorySize(0), priority(128), 
                     lastAccessed(0), state(RESOURCE_UNLOADED), data(nullptr) {}
};

// Level chunk - contains only what's needed for current area
struct LevelChunk {
    uint16_t chunkId;
    int16_t worldX, worldY;     // World coordinates this chunk covers
    uint16_t width, height;     // Chunk dimensions
    
    // Resources needed for this chunk
    std::vector<uint16_t> requiredSprites;
    std::vector<uint16_t> requiredAudio;
    std::vector<uint16_t> requiredPalettes;
    
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
    std::vector<ChunkEntity> entities;
    
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
    std::map<uint16_t, ResourceInfo> resourceRegistry;
    
    // Currently loaded resources
    std::vector<uint16_t> loadedResources;
    
    // Level chunk system
    std::map<uint16_t, LevelChunk> levelChunks;
    std::vector<uint16_t> loadedChunks;
    
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
        maxMemoryUsage(128 * 1024),      // 128KB budget
        currentMemoryUsage(0),
        memoryPressureThreshold(96 * 1024), // Start unloading at 96KB
        playerX(0), playerY(0),
        loadRadius(32),                   // Load chunks within 32 pixels
        loadTime(0), unloadTime(0), memoryFragmentation(0) {}
    
    // Resource registry management
    bool registerResource(uint16_t resourceId, ResourceType type, 
                         const String& filePath, uint32_t offset, uint32_t size);
    
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
    std::vector<uint16_t> getLoadedResources() const { return loadedResources; }
    
private:
    // Internal loading/unloading
    bool loadResourceFromFile(ResourceInfo& info);
    void unloadResourceFromMemory(ResourceInfo& info);
    
    // Memory pressure handling
    bool freeMemoryForResource(uint32_t requiredBytes);
    uint16_t findLeastRecentlyUsedResource();
    
    // Chunk proximity calculations
    bool isChunkInRange(const LevelChunk& chunk, int16_t centerX, int16_t centerY, uint16_t radius);
    std::vector<uint16_t> getChunksInRange(int16_t centerX, int16_t centerY, uint16_t radius);
    
    // File I/O helpers
    bool loadDataFromFile(const String& filePath, uint32_t offset, uint32_t size, void* buffer);
};

// Implementation of key methods
inline void* LazyResourceManager::getResource(uint16_t resourceId) {
    auto it = resourceRegistry.find(resourceId);
    if (it == resourceRegistry.end()) {
        return nullptr;
    }
    
    ResourceInfo& info = it->second;
    
    // Update access time
    info.lastAccessed = millis();
    
    // If already loaded, return data
    if (info.state == RESOURCE_LOADED && info.data) {
        return info.data;
    }
    
    // If not loaded, load it now
    if (info.state == RESOURCE_UNLOADED) {
        if (loadResourceFromFile(info)) {
            return info.data;
        }
    }
    
    return nullptr;
}

inline bool LazyResourceManager::loadResourceFromFile(ResourceInfo& info) {
    // Check if we have enough memory
    if (currentMemoryUsage + info.memorySize > maxMemoryUsage) {
        if (!freeMemoryForResource(info.memorySize)) {
            Serial.print("ERROR: Cannot free memory for resource ");
            Serial.println(info.resourceId);
            return false;
        }
    }
    
    info.state = RESOURCE_LOADING;
    
    // Allocate memory
    info.data = malloc(info.memorySize);
    if (!info.data) {
        Serial.print("ERROR: Memory allocation failed for resource ");
        Serial.println(info.resourceId);
        info.state = RESOURCE_ERROR;
        return false;
    }
    
    // Load from file
    uint32_t startTime = micros();
    bool success = loadDataFromFile(info.filePath, info.fileOffset, info.fileSize, info.data);
    loadTime += micros() - startTime;
    
    if (success) {
        info.state = RESOURCE_LOADED;
        currentMemoryUsage += info.memorySize;
        loadedResources.push_back(info.resourceId);
        
        Serial.print("Loaded resource ");
        Serial.print(info.resourceId);
        Serial.print(" (");
        Serial.print(info.memorySize);
        Serial.println(" bytes)");
        
        return true;
    } else {
        free(info.data);
        info.data = nullptr;
        info.state = RESOURCE_ERROR;
        
        Serial.print("ERROR: Failed to load resource ");
        Serial.println(info.resourceId);
        return false;
    }
}

inline void LazyResourceManager::unloadResourceFromMemory(ResourceInfo& info) {
    if (info.data) {
        uint32_t startTime = micros();
        
        free(info.data);
        info.data = nullptr;
        info.state = RESOURCE_UNLOADED;
        currentMemoryUsage -= info.memorySize;
        
        // Remove from loaded list
        auto it = std::find(loadedResources.begin(), loadedResources.end(), info.resourceId);
        if (it != loadedResources.end()) {
            loadedResources.erase(it);
        }
        
        unloadTime += micros() - startTime;
        
        Serial.print("Unloaded resource ");
        Serial.print(info.resourceId);
        Serial.print(" (freed ");
        Serial.print(info.memorySize);
        Serial.println(" bytes)");
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
    std::vector<uint16_t> chunksInRange = getChunksInRange(playerX, playerY, loadRadius);
    
    // Load chunks that aren't loaded
    for (uint16_t chunkId : chunksInRange) {
        auto it = std::find(loadedChunks.begin(), loadedChunks.end(), chunkId);
        if (it == loadedChunks.end()) {
            loadChunk(chunkId);
        }
    }
    
    // Unload chunks that are too far away
    for (auto it = loadedChunks.begin(); it != loadedChunks.end();) {
        uint16_t chunkId = *it;
        auto chunkIt = levelChunks.find(chunkId);
        
        if (chunkIt != levelChunks.end()) {
            const LevelChunk& chunk = chunkIt->second;
            
            if (!isChunkInRange(chunk, playerX, playerY, loadRadius * 1.5f)) {
                unloadChunk(chunkId);
                it = loadedChunks.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
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
    while (freedBytes < requiredBytes && !loadedResources.empty()) {
        uint16_t lruResourceId = findLeastRecentlyUsedResource();
        if (lruResourceId == 0xFFFF) break; // No more resources to unload
        
        auto it = resourceRegistry.find(lruResourceId);
        if (it != resourceRegistry.end()) {
            freedBytes += it->second.memorySize;
            unloadResourceFromMemory(it->second);
        }
    }
    
    return freedBytes >= requiredBytes;
}

inline void LazyResourceManager::printMemoryStatus() {
    Serial.println("=== Lazy Resource Manager Status ===");
    Serial.print("Memory Usage: ");
    Serial.print(currentMemoryUsage);
    Serial.print(" / ");
    Serial.print(maxMemoryUsage);
    Serial.print(" bytes (");
    Serial.print((currentMemoryUsage * 100) / maxMemoryUsage);
    Serial.println("%)");
    
    Serial.print("Loaded Resources: ");
    Serial.println(loadedResources.size());
    
    Serial.print("Loaded Chunks: ");
    Serial.println(loadedChunks.size());
    
    Serial.print("Player Position: (");
    Serial.print(playerX);
    Serial.print(", ");
    Serial.print(playerY);
    Serial.println(")");
    
    Serial.print("Load Radius: ");
    Serial.println(loadRadius);
    
    Serial.println("====================================");
}
