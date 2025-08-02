#include "partitioned_system.h"
#include <cstring>
#include <algorithm>

// Magic numbers for partition validation
#define PARTITION_MAGIC 0xDB01
#define ENTRY_MAGIC     0xDA7A

// Static memory allocation (RTC data for persistence)
RTC_DATA_ATTR uint8_t PartitionedDatabase::lpSramData[LP_SRAM_SIZE];

// Global database instance
PartitionedDatabase database;

PartitionedDatabase::PartitionedDatabase() : initialized(false), cacheSize(0), cacheCount(0) {
    // Initialize all pointers to safe defaults
    romPartition = nullptr;
    savePartition = nullptr;
    backupPartition = nullptr;
    runtimePartition = nullptr;
    cache = nullptr;
    
    // Clear configuration
    memset(&config, 0, sizeof(config));
    romSize = saveSize = backupSize = runtimeSize = 0;
}

ErrorCode PartitionedDatabase::initialize(const PartitionConfig* partitionConfig) {
    if (initialized) {
        return ERROR_NOT_INITIALIZED; // Already initialized
    }
    
    // Use default configuration if none provided
    if (!partitionConfig) {
        static const PartitionConfig defaultConfig = {
            .romSize = ROM_PARTITION_SIZE,
            .saveSize = SAVE_PARTITION_SIZE,
            .backupSize = BACKUP_PARTITION_SIZE,
            .runtimeSize = RUNTIME_PARTITION_SIZE,
            .enableCompression = false,
            .enableEncryption = false,
            .maxCacheEntries = 8,
            .safetyLevel = 1
        };
        partitionConfig = &defaultConfig;
    }
    
    // Validate configuration thoroughly
    if (!VALIDATE_CONFIG(partitionConfig)) {
        return ERROR_INVALID_CONFIG;
    }
    
    // Store configuration
    config = *partitionConfig;
    
    // Calculate partition layout with safety checks
    ErrorCode result = setupPartitions();
    if (result != SUCCESS) {
        return result;
    }
    
    // Initialize cache if requested
    if (config.maxCacheEntries > 0) {
        cacheSize = std::min((uint8_t)config.maxCacheEntries, (uint8_t)64); // Max 64 entries
        size_t cacheMemory = cacheSize * sizeof(CacheEntry);
        
        // Ensure cache fits in runtime partition
        if (cacheMemory < runtimeSize) {
            cache = reinterpret_cast<CacheEntry*>(runtimePartition + runtimeSize - cacheMemory);
            memset(cache, 0, cacheMemory);
            runtimeSize -= cacheMemory; // Reduce usable runtime space
        } else {
            cacheSize = 0; // Disable cache if it doesn't fit
        }
    }
    
    // Initialize partition headers
    result = initializePartitionHeaders();
    if (result != SUCCESS) {
        cleanup();
        return result;
    }
    
    initialized = true;
    return SUCCESS;
}

ErrorCode PartitionedDatabase::setupPartitions() {
    // Calculate partition offsets with alignment
    uint16_t offset = 0;
    
    // ROM partition (first, aligned)
    romPartition = lpSramData + offset;
    romSize = config.romSize;
    offset += romSize;
    
    // Save partition
    savePartition = lpSramData + offset;
    saveSize = config.saveSize;
    offset += saveSize;
    
    // Backup partition
    backupPartition = lpSramData + offset;
    backupSize = config.backupSize;
    offset += backupSize;
    
    // Runtime partition (remainder)
    runtimePartition = lpSramData + offset;
    runtimeSize = config.runtimeSize;
    offset += runtimeSize;
    
    // Final bounds check
    if (offset > LP_SRAM_SIZE) {
        return ERROR_BUFFER_OVERFLOW;
    }
    
    return SUCCESS;
}

ErrorCode PartitionedDatabase::initializePartitionHeaders() {
    PartitionHeader* headers[] = {
        reinterpret_cast<PartitionHeader*>(romPartition),
        reinterpret_cast<PartitionHeader*>(savePartition),
        reinterpret_cast<PartitionHeader*>(backupPartition),
        reinterpret_cast<PartitionHeader*>(runtimePartition)
    };
    
    uint16_t sizes[] = { romSize, saveSize, backupSize, runtimeSize };
    
    for (int i = 0; i < 4; i++) {
        if (sizes[i] < sizeof(PartitionHeader)) {
            return ERROR_INVALID_CONFIG;
        }
        
        PartitionHeader* header = headers[i];
        header->magic = PARTITION_MAGIC;
        header->version = DATABASE_VERSION;
        header->entryCount = 0;
        header->usedBytes = sizeof(PartitionHeader);
        header->totalSize = sizes[i];
        header->checksum = 0; // Will be calculated later
        header->reserved = 0;
        
        // Clear data area
        uint8_t* dataArea = reinterpret_cast<uint8_t*>(header) + sizeof(PartitionHeader);
        size_t dataSize = sizes[i] - sizeof(PartitionHeader);
        memset(dataArea, 0, dataSize);
    }
    
    return SUCCESS;
}

ErrorCode PartitionedDatabase::validatePointer(const void* ptr, uint16_t size, uint8_t partition) {
    if (!ptr || size == 0) {
        return ERROR_INVALID_PARTITION;
    }
    
    uint8_t* partitionStart = getPartitionStart(partition);
    uint16_t partitionSize = getPartitionSize(partition);
    
    if (!partitionStart) {
        return ERROR_INVALID_PARTITION;
    }
    
    const uint8_t* bytePtr = reinterpret_cast<const uint8_t*>(ptr);
    
    // Check if pointer and size are within partition bounds
    if (bytePtr < partitionStart || 
        bytePtr + size > partitionStart + partitionSize) {
        return ERROR_BUFFER_OVERFLOW;
    }
    
    return SUCCESS;
}

ErrorCode PartitionedDatabase::validateEntry(uint32_t key, uint16_t size) {
    // Check key validity
    if (!isValidKey(key)) {
        return ERROR_INVALID_KEY;
    }
    
    // Check size limits
    if (!ENTRY_SIZE_VALID(size)) {
        return ERROR_ENTRY_TOO_LARGE;
    }
    
    return SUCCESS;
}

bool PartitionedDatabase::isValidKey(uint32_t key) {
    // Key cannot be zero or all 1s
    if (key == 0 || key == 0xFFFFFFFF) {
        return false;
    }
    
    // Extract components
    uint8_t ns = GET_NAMESPACE(key);
    uint8_t cat = GET_CATEGORY(key);
    uint16_t id = GET_ID(key);
    
    // Basic sanity checks
    return (ns != 0xFF && cat != 0xFF && id != 0xFFFF);
}

ErrorCode PartitionedDatabase::set(uint32_t key, const void* data, uint8_t size, uint8_t type) {
    if (!initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    // Validate inputs
    ErrorCode result = validateEntry(key, size);
    if (result != SUCCESS) {
        return result;
    }
    
    if (!data && size > 0) {
        return ERROR_INVALID_PARTITION;
    }
    
    // Use save partition by default
    return writeEntryInternal(key, data, size, type, PARTITION_SAVE, 0);
}

ErrorCode PartitionedDatabase::writeEntryInternal(uint32_t key, const void* data, uint8_t size, 
                                                 uint8_t type, uint8_t partition, uint8_t flags) {
    uint8_t* partitionStart = getPartitionStart(partition);
    uint16_t partitionSize = getPartitionSize(partition);
    
    if (!partitionStart) {
        return ERROR_INVALID_PARTITION;
    }
    
    PartitionHeader* header = reinterpret_cast<PartitionHeader*>(partitionStart);
    
    // Check if partition is full
    uint16_t requiredSpace = sizeof(EntryHeader) + size;
    uint16_t availableSpace = partitionSize - header->usedBytes;
    
    if (requiredSpace > availableSpace) {
        return ERROR_PARTITION_FULL;
    }
    
    // Check entry count limit
    if (header->entryCount >= MAX_ENTRIES_PER_PARTITION) {
        return ERROR_INDEX_OVERFLOW;
    }
    
    // Find write position (after existing entries)
    uint8_t* writePos = partitionStart + header->usedBytes;
    
    // Validate write position is safe
    ErrorCode result = validatePointer(writePos, requiredSpace, partition);
    if (result != SUCCESS) {
        return result;
    }
    
    // Create entry header
    EntryHeader* entry = reinterpret_cast<EntryHeader*>(writePos);
    entry->key = key;
    entry->type_and_flags = (type << 4) | (flags & 0x0F);
    entry->size = size;
    
    // Copy data safely
    if (size > 0 && data) {
        uint8_t* dataPtr = writePos + sizeof(EntryHeader);
        memcpy(dataPtr, data, size);
    }
    
    // Update partition header
    header->entryCount++;
    header->usedBytes += requiredSpace;
    updatePartitionChecksum(partition);
    
    // Update cache if enabled
    if (cache && cacheSize > 0) {
        uint16_t offset = writePos - partitionStart;
        cacheEntry(key, requiredSpace, offset);
    }
    
    return SUCCESS;
}

ErrorCode PartitionedDatabase::get(uint32_t key, void* buffer, uint8_t maxSize, uint8_t* actualSize) {
    if (!initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    if (!buffer || maxSize == 0) {
        return ERROR_INVALID_PARTITION;
    }
    
    // Search partitions in priority order: runtime -> save -> backup -> rom
    uint8_t searchOrder[] = { 
        PARTITION_RUNTIME, 
        PARTITION_SAVE, 
        PARTITION_BACKUP, 
        PARTITION_ROM 
    };
    
    for (int i = 0; i < 4; i++) {
        ErrorCode result = readEntryInternal(key, buffer, maxSize, actualSize, searchOrder[i]);
        if (result == SUCCESS) {
            return SUCCESS;
        }
    }
    
    return ERROR_KEY_NOT_FOUND;
}

ErrorCode PartitionedDatabase::readEntryInternal(uint32_t key, void* buffer, uint8_t maxSize, 
                                                uint8_t* actualSize, uint8_t partition) {
    uint8_t* partitionStart = getPartitionStart(partition);
    
    if (!partitionStart) {
        return ERROR_INVALID_PARTITION;
    }
    
    PartitionHeader* header = reinterpret_cast<PartitionHeader*>(partitionStart);
    uint8_t* searchPtr = partitionStart + sizeof(PartitionHeader);
    
    // Linear search through entries
    for (uint8_t i = 0; i < header->entryCount; i++) {
        // Bounds check before accessing entry
        if (searchPtr + sizeof(EntryHeader) > partitionStart + header->totalSize) {
            return ERROR_MEMORY_CORRUPTED;
        }
        
        EntryHeader* entry = reinterpret_cast<EntryHeader*>(searchPtr);
        
        if (entry->key == key) {
            // Found the entry
            uint8_t entrySize = entry->size;
            
            if (actualSize) {
                *actualSize = entrySize;
            }
            
            if (entrySize > maxSize) {
                return ERROR_BUFFER_OVERFLOW;
            }
            
            // Bounds check data area
            uint8_t* dataPtr = searchPtr + sizeof(EntryHeader);
            if (dataPtr + entrySize > partitionStart + header->totalSize) {
                return ERROR_MEMORY_CORRUPTED;
            }
            
            // Copy data safely
            if (entrySize > 0) {
                memcpy(buffer, dataPtr, entrySize);
            }
            
            return SUCCESS;
        }
        
        // Move to next entry
        uint16_t entryTotalSize = sizeof(EntryHeader) + entry->size;
        searchPtr += entryTotalSize;
        
        // Bounds check for next iteration
        if (searchPtr > partitionStart + header->totalSize) {
            return ERROR_MEMORY_CORRUPTED;
        }
    }
    
    return ERROR_KEY_NOT_FOUND;
}

uint8_t* PartitionedDatabase::getPartitionStart(uint8_t partitionId) {
    switch (partitionId) {
        case PARTITION_ROM: return romPartition;
        case PARTITION_SAVE: return savePartition;
        case PARTITION_BACKUP: return backupPartition;
        case PARTITION_RUNTIME: return runtimePartition;
        default: return nullptr;
    }
}

uint16_t PartitionedDatabase::getPartitionSize(uint8_t partitionId) {
    switch (partitionId) {
        case PARTITION_ROM: return romSize;
        case PARTITION_SAVE: return saveSize;
        case PARTITION_BACKUP: return backupSize;
        case PARTITION_RUNTIME: return runtimeSize;
        default: return 0;
    }
}

void PartitionedDatabase::updatePartitionChecksum(uint8_t partitionId) {
    uint8_t* partitionStart = getPartitionStart(partitionId);
    uint16_t partitionSize = getPartitionSize(partitionId);
    
    if (!partitionStart) return;
    
    PartitionHeader* header = reinterpret_cast<PartitionHeader*>(partitionStart);
    
    // Calculate checksum of data area only
    uint8_t* dataStart = partitionStart + sizeof(PartitionHeader);
    uint16_t dataSize = header->usedBytes - sizeof(PartitionHeader);
    
    header->checksum = calculateChecksum(dataStart, dataSize);
}

uint32_t PartitionedDatabase::calculateChecksum(const void* data, uint16_t size) {
    // Simple CRC32-like checksum
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
    uint32_t crc = 0xFFFFFFFF;
    
    for (uint16_t i = 0; i < size; i++) {
        crc ^= bytes[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x00000001) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return ~crc;
}

void PartitionedDatabase::cacheEntry(uint32_t key, uint16_t size, uint16_t partition_offset) {
    if (!cache || cacheSize == 0) return;
    
    // Find empty slot or LRU entry
    uint8_t targetSlot = 0;
    uint32_t oldestTime = 0xFFFFFFFF;
    
    for (uint8_t i = 0; i < cacheSize; i++) {
        if (cache[i].key == 0) {
            // Empty slot found
            targetSlot = i;
            break;
        }
        
        if (cache[i].access_time < oldestTime) {
            oldestTime = cache[i].access_time;
            targetSlot = i;
        }
    }
    
    // Update cache entry
    cache[targetSlot].key = key;
    cache[targetSlot].size = size;
    cache[targetSlot].partition_offset = partition_offset;
    cache[targetSlot].access_time = millis(); // Current timestamp
    
    if (cacheCount < cacheSize) {
        cacheCount++;
    }
}

void PartitionedDatabase::cleanup() {
    initialized = false;
    cache = nullptr;
    cacheSize = cacheCount = 0;
    
    // Clear pointers (memory is static, don't free)
    romPartition = savePartition = backupPartition = runtimePartition = nullptr;
    romSize = saveSize = backupSize = runtimeSize = 0;
}

// Simple type-safe accessors
ErrorCode PartitionedDatabase::setU8(uint32_t key, uint8_t value) {
    return set(key, &value, sizeof(value), ENTRY_U8);
}

ErrorCode PartitionedDatabase::setU16(uint32_t key, uint16_t value) {
    return set(key, &value, sizeof(value), ENTRY_U16);
}

ErrorCode PartitionedDatabase::setU32(uint32_t key, uint32_t value) {
    return set(key, &value, sizeof(value), ENTRY_U32);
}

uint8_t PartitionedDatabase::getU8(uint32_t key, uint8_t defaultValue) {
    uint8_t value = defaultValue;
    get(key, &value, sizeof(value));
    return value;
}

uint16_t PartitionedDatabase::getU16(uint32_t key, uint16_t defaultValue) {
    uint16_t value = defaultValue;
    get(key, &value, sizeof(value));
    return value;
}

uint32_t PartitionedDatabase::getU32(uint32_t key, uint32_t defaultValue) {
    uint32_t value = defaultValue;
    get(key, &value, sizeof(value));
    return value;
}

// Memory monitoring functions
uint16_t PartitionedDatabase::getTotalUsedBytes() {
    if (!initialized) return 0;
    
    uint16_t total = 0;
    for (uint8_t i = 0; i < 4; i++) {
        total += getPartitionUsedBytes(i);
    }
    return total;
}

uint16_t PartitionedDatabase::getTotalFreeBytes() {
    if (!initialized) return 0;
    
    return LP_SRAM_SIZE - getTotalUsedBytes();
}

uint16_t PartitionedDatabase::getPartitionUsedBytes(uint8_t partitionId) {
    uint8_t* partitionStart = getPartitionStart(partitionId);
    if (!partitionStart) return 0;
    
    PartitionHeader* header = reinterpret_cast<PartitionHeader*>(partitionStart);
    return header->usedBytes;
}

uint16_t PartitionedDatabase::getPartitionFreeBytes(uint8_t partitionId) {
    uint16_t partitionSize = getPartitionSize(partitionId);
    uint16_t usedBytes = getPartitionUsedBytes(partitionId);
    
    return (partitionSize > usedBytes) ? (partitionSize - usedBytes) : 0;
}

bool PartitionedDatabase::exists(uint32_t key) {
    uint8_t dummy;
    return get(key, &dummy, sizeof(dummy)) == SUCCESS;
}

bool PartitionedDatabase::validateDatabase() {
    if (!initialized) return false;
    
    // Validate each partition
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t* partitionStart = getPartitionStart(i);
        if (!partitionStart) continue;
        
        PartitionHeader* header = reinterpret_cast<PartitionHeader*>(partitionStart);
        
        // Check magic number
        if (header->magic != PARTITION_MAGIC) {
            return false;
        }
        
        // Check bounds
        if (header->usedBytes > header->totalSize) {
            return false;
        }
        
        // Verify checksum
        uint8_t* dataStart = partitionStart + sizeof(PartitionHeader);
        uint16_t dataSize = header->usedBytes - sizeof(PartitionHeader);
        uint32_t expectedChecksum = calculateChecksum(dataStart, dataSize);
        
        if (header->checksum != expectedChecksum) {
            return false;
        }
    }
    
    return true;
}

void PartitionedDatabase::printMemoryMap() {
    if (!initialized) {
        Serial.println("Database not initialized");
        return;
    }
    
    Serial.println("=== Database Memory Map ===");
    Serial.printf("Total LP-SRAM: %d bytes\n", LP_SRAM_SIZE);
    Serial.printf("Total Used: %d bytes (%.1f%%)\n", 
                  getTotalUsedBytes(), 
                  (getTotalUsedBytes() * 100.0f) / LP_SRAM_SIZE);
    Serial.printf("Total Free: %d bytes\n", getTotalFreeBytes());
    Serial.println();
    
    const char* partitionNames[] = {"ROM", "Save", "Backup", "Runtime"};
    for (uint8_t i = 0; i < 4; i++) {
        uint16_t size = getPartitionSize(i);
        uint16_t used = getPartitionUsedBytes(i);
        uint16_t free = getPartitionFreeBytes(i);
        uint8_t entries = getEntryCount(i);
        
        Serial.printf("%s: %d/%d bytes (%.1f%%), %d entries\n",
                      partitionNames[i], used, size, 
                      (used * 100.0f) / size, entries);
    }
    
    if (cache) {
        Serial.printf("Cache: %d/%d entries\n", cacheCount, cacheSize);
    }
}

uint8_t PartitionedDatabase::getEntryCount(uint8_t partitionId) {
    uint8_t* partitionStart = getPartitionStart(partitionId);
    if (!partitionStart) return 0;
    
    PartitionHeader* header = reinterpret_cast<PartitionHeader*>(partitionStart);
    return header->entryCount;
}
