// Sprite Batch System - ESP32 16x16 Chunk-Based Sprite Processing
// Processes sprites into consistent 16x16 memory chunks with auto-padding and flip operations
// Memory-optimized for ESP32 with scalable batching architecture

#pragma once
#include <stdint.h>
#include "../engine_common.h"
#include "fallback_asset_system.h"

namespace WispEngine {
namespace Graphics {

// 16x16 chunk processing constants
#define SPRITE_BATCH_CHUNK_SIZE 16
#define SPRITE_BATCH_CHUNK_PIXELS (SPRITE_BATCH_CHUNK_SIZE * SPRITE_BATCH_CHUNK_SIZE)
#define SPRITE_BATCH_CHUNK_BYTES SPRITE_BATCH_CHUNK_PIXELS // 1 byte per pixel (palette index)
#define MAX_SPRITE_BATCH_CHUNKS 64 // Maximum chunks per sprite (for very large sprites)

// Memory pool for consistent chunk allocation
#define SPRITE_BATCH_POOL_SIZE 32 // Number of 16x16 chunks in pool
#define SPRITE_BATCH_POOL_BYTES (SPRITE_BATCH_POOL_SIZE * SPRITE_BATCH_CHUNK_BYTES)

// Sprite flip operations
enum SpriteFlipMode {
    FLIP_NONE = 0,
    FLIP_HORIZONTAL = 1,
    FLIP_VERTICAL = 2,
    FLIP_BOTH = 3
};

// Enhanced sprite header for batched processing
struct BatchedSpriteHeader {
    uint32_t magic;                  // 'WBAT' - Wisp Batched sprite format
    SpriteArtType artType;           // Splash, Entity, Tile, UI
    uint16_t originalWidth;          // Original sprite dimensions
    uint16_t originalHeight;
    uint16_t chunksWidth;            // Number of 16x16 chunks horizontally
    uint16_t chunksHeight;           // Number of 16x16 chunks vertically
    uint16_t totalChunks;            // Total number of chunks
    uint8_t paddingColor;            // Transparent color index for padding
    uint8_t flags;                   // Compression, animation flags
    uint32_t chunkDataOffset;        // Offset to chunk data
    uint32_t totalDataSize;          // Total size of all chunk data
    
    // Animation data (for entities)
    struct AnimationInfo {
        uint8_t frameCount;          // Number of animation frames
        uint8_t framesPerRow;        // Sprite sheet layout
        uint8_t defaultFPS;          // Default animation speed
        uint8_t loopMode;            // Loop, once, ping-pong
    } animation;
    
    uint8_t reserved[8];             // Future expansion
} __attribute__((packed));

// 16x16 chunk descriptor
struct SpriteChunk {
    uint16_t chunkId;                // Unique chunk identifier
    uint8_t x, y;                    // Chunk position in sprite (chunk coordinates)
    uint8_t data[SPRITE_BATCH_CHUNK_BYTES]; // 16x16 palette indices
    uint8_t transparentPixels;       // Number of transparent pixels (for optimization)
    uint8_t flags;                   // Chunk flags (empty, solid, etc.)
} __attribute__((packed));

// Memory pool for consistent chunk allocation
class SpriteChunkPool {
private:
    uint8_t memoryPool[SPRITE_BATCH_POOL_BYTES];
    bool chunkAllocated[SPRITE_BATCH_POOL_SIZE];
    uint8_t allocatedCount;
    uint32_t allocationTime[SPRITE_BATCH_POOL_SIZE]; // For LRU eviction
    
public:
    SpriteChunkPool();
    
    // Allocate a 16x16 chunk from the pool
    uint8_t* allocateChunk(uint16_t chunkId);
    
    // Free a chunk back to the pool
    void freeChunk(uint16_t chunkId);
    
    // Get chunk data by ID
    uint8_t* getChunk(uint16_t chunkId);
    
    // Memory management
    void freeOldestChunk(); // LRU eviction
    uint8_t getFreeChunks() const { return SPRITE_BATCH_POOL_SIZE - allocatedCount; }
    uint8_t getAllocatedCount() const { return allocatedCount; }
    
    // Debug
    void printPoolStats() const;
};

// Sprite batch processor - converts sprites to 16x16 chunks
class SpriteBatchProcessor {
private:
    SpriteChunkPool chunkPool;
    
    // Processed sprites cache
    struct ProcessedSprite {
        BatchedSpriteHeader header;
        SpriteChunk* chunks[MAX_SPRITE_BATCH_CHUNKS];
        uint8_t chunkCount;
        bool cached;
        uint32_t lastAccessed;
    };
    
    static const uint8_t MAX_CACHED_SPRITES = 16;
    ProcessedSprite cachedSprites[MAX_CACHED_SPRITES];
    uint8_t cachedCount;
    
public:
    SpriteBatchProcessor();
    ~SpriteBatchProcessor();
    
    // === MAIN PROCESSING API ===
    
    // Process raw sprite data into 16x16 chunks with auto-padding
    bool processSpriteToChunks(const uint8_t* rawSpriteData, uint32_t dataSize, 
                              SpriteArtType artType, uint16_t spriteId);
    
    // Get processed sprite chunks
    const ProcessedSprite* getProcessedSprite(uint16_t spriteId);
    
    // === CHUNK MANIPULATION ===
    
    // Apply flip operations to chunks
    void flipSpriteChunks(uint16_t spriteId, SpriteFlipMode flipMode);
    
    // Create flipped variant without modifying original
    uint16_t createFlippedVariant(uint16_t originalSpriteId, SpriteFlipMode flipMode);
    
    // === AUTO-PADDING ===
    
    // Fill incomplete chunks with transparent pixels
    void padChunkWithTransparency(SpriteChunk* chunk, uint8_t transparentColor);
    
    // Auto-detect best padding strategy based on sprite content
    uint8_t detectBestPaddingColor(const uint8_t* spriteData, uint16_t width, uint16_t height);
    
    // === MEMORY MANAGEMENT ===
    
    // Free cached sprite from memory
    void evictSprite(uint16_t spriteId);
    
    // Force garbage collection of old sprites
    void garbageCollect();
    
    // Get memory usage statistics
    uint32_t getMemoryUsage() const;
    uint32_t getChunkPoolUsage() const { return chunkPool.getAllocatedCount() * SPRITE_BATCH_CHUNK_BYTES; }
    
    // === RENDERING HELPERS ===
    
    // Render sprite chunk to target buffer
    void renderChunk(const SpriteChunk* chunk, uint8_t* targetBuffer, uint16_t targetWidth,
                    uint16_t targetX, uint16_t targetY, SpriteFlipMode flipMode = FLIP_NONE);
    
    // Render entire batched sprite
    void renderBatchedSprite(uint16_t spriteId, uint8_t* targetBuffer, uint16_t targetWidth,
                            uint16_t targetX, uint16_t targetY, SpriteFlipMode flipMode = FLIP_NONE);
    
    // === DEBUGGING ===
    
    void printBatchStats() const;
    void printSpriteInfo(uint16_t spriteId) const;
    
private:
    // === INTERNAL PROCESSING ===
    
    // Convert raw sprite to chunks
    void convertToChunks(const uint8_t* pixelData, uint16_t width, uint16_t height,
                        ProcessedSprite* sprite, uint8_t transparentColor);
    
    // Process individual 16x16 region into chunk
    void processChunkRegion(const uint8_t* sourceData, uint16_t sourceWidth, uint16_t sourceHeight,
                           uint16_t chunkX, uint16_t chunkY, SpriteChunk* targetChunk, 
                           uint8_t transparentColor);
    
    // === FLIP OPERATIONS ===
    
    // Flip single chunk horizontally
    void flipChunkHorizontal(SpriteChunk* chunk);
    
    // Flip single chunk vertically
    void flipChunkVertical(SpriteChunk* chunk);
    
    // Flip both axes
    void flipChunkBoth(SpriteChunk* chunk);
    
    // === CACHE MANAGEMENT ===
    
    // Find cached sprite
    ProcessedSprite* findCachedSprite(uint16_t spriteId);
    
    // Add sprite to cache
    bool addToCache(uint16_t spriteId, const ProcessedSprite* sprite);
    
    // Remove oldest sprite from cache
    void evictOldestCached();
    
    // === OPTIMIZATION ===
    
    // Detect if chunk is completely transparent
    bool isChunkEmpty(const SpriteChunk* chunk, uint8_t transparentColor);
    
    // Detect if chunk is solid color
    bool isChunkSolid(const SpriteChunk* chunk, uint8_t* solidColor);
    
    // Compress chunk data if beneficial
    bool compressChunk(SpriteChunk* chunk);
};

// Global sprite batch processor instance
extern SpriteBatchProcessor spriteBatchProcessor;

// === UTILITY FUNCTIONS ===

// Calculate number of chunks needed for dimensions
inline uint16_t calculateChunksWidth(uint16_t pixelWidth) {
    return (pixelWidth + SPRITE_BATCH_CHUNK_SIZE - 1) / SPRITE_BATCH_CHUNK_SIZE;
}

inline uint16_t calculateChunksHeight(uint16_t pixelHeight) {
    return (pixelHeight + SPRITE_BATCH_CHUNK_SIZE - 1) / SPRITE_BATCH_CHUNK_SIZE;
}

inline uint16_t calculateTotalChunks(uint16_t pixelWidth, uint16_t pixelHeight) {
    return calculateChunksWidth(pixelWidth) * calculateChunksHeight(pixelHeight);
}

// Convert chunk coordinates to pixel coordinates
inline uint16_t chunkToPixelX(uint8_t chunkX) {
    return chunkX * SPRITE_BATCH_CHUNK_SIZE;
}

inline uint16_t chunkToPixelY(uint8_t chunkY) {
    return chunkY * SPRITE_BATCH_CHUNK_SIZE;
}

// Convert pixel coordinates to chunk coordinates
inline uint8_t pixelToChunkX(uint16_t pixelX) {
    return pixelX / SPRITE_BATCH_CHUNK_SIZE;
}

inline uint8_t pixelToChunkY(uint16_t pixelY) {
    return pixelY / SPRITE_BATCH_CHUNK_SIZE;
}

// Magic numbers for batched sprite format
#define MAGIC_BATCHED_SPRITE 0x54414257  // 'WBAT'

} // namespace Graphics
} // namespace WispEngine
