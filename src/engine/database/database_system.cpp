#include "database_system.h"
#include <string.h>
#include <utility>   // For std::pair
#include <algorithm>

WispPartitionedDB::WispPartitionedDB() : initialized(false), config(nullptr) {
    // Initialize all partition pointers to null
    for (int i = 0; i < WISP_DB_PARTITION_COUNT; i++) {
        partitions[i] = nullptr;
        partitionSizes[i] = 0;
    }
}

WispPartitionedDB::~WispPartitionedDB() {
    cleanup();
}

WispErrorCode WispPartitionedDB::initialize(const WispPartitionConfig* cfg) {
    if (initialized) {
        return WISP_ERROR_ALREADY_INITIALIZED;
    }
    
    if (!cfg) {
        return WISP_ERROR_INVALID_CONFIG;
    }
    
    config = cfg;
    
    // Validate configuration
    if (config->romSize == 0 || config->saveSize == 0) {
        return WISP_ERROR_INVALID_CONFIG;
    }
    
    // Allocate partitions based on configuration
    WispErrorCode result = allocatePartitions();
    if (result != WISP_SUCCESS) {
        cleanup();
        return result;
    }
    
    // Initialize partition headers
    result = initializePartitions();
    if (result != WISP_SUCCESS) {
        cleanup();
        return result;
    }
    
    initialized = true;
    return WISP_SUCCESS;
}

WispErrorCode WispPartitionedDB::allocatePartitions() {
    // Calculate total memory needed
    size_t totalSize = config->romSize + config->saveSize + 
                       config->backupSize + config->runtimeSize;
    
    // Validate we don't exceed maximum partition size
    if (totalSize > WISP_DB_MAX_TOTAL_SIZE) {
        return WISP_ERROR_MEMORY_EXCEEDED;
    }
    
    // Allocate ROM partition
    partitions[WISP_DB_PARTITION_ROM] = (uint8_t*)malloc(config->romSize);
    if (!partitions[WISP_DB_PARTITION_ROM]) {
        return WISP_ERROR_OUT_OF_MEMORY;
    }
    partitionSizes[WISP_DB_PARTITION_ROM] = config->romSize;
    
    // Allocate Save partition
    partitions[WISP_DB_PARTITION_SAVE] = (uint8_t*)malloc(config->saveSize);
    if (!partitions[WISP_DB_PARTITION_SAVE]) {
        return WISP_ERROR_OUT_OF_MEMORY;
    }
    partitionSizes[WISP_DB_PARTITION_SAVE] = config->saveSize;
    
    // Allocate Backup partition (if specified)
    if (config->backupSize > 0) {
        partitions[WISP_DB_PARTITION_BACKUP] = (uint8_t*)malloc(config->backupSize);
        if (!partitions[WISP_DB_PARTITION_BACKUP]) {
            return WISP_ERROR_OUT_OF_MEMORY;
        }
        partitionSizes[WISP_DB_PARTITION_BACKUP] = config->backupSize;
    }
    
    // Allocate Runtime partition (if specified)
    if (config->runtimeSize > 0) {
        partitions[WISP_DB_PARTITION_RUNTIME] = (uint8_t*)malloc(config->runtimeSize);
        if (!partitions[WISP_DB_PARTITION_RUNTIME]) {
            return WISP_ERROR_OUT_OF_MEMORY;
        }
        partitionSizes[WISP_DB_PARTITION_RUNTIME] = config->runtimeSize;
    }
    
    return WISP_SUCCESS;
}

WispErrorCode WispPartitionedDB::initializePartitions() {
    // Initialize each partition with header
    for (int i = 0; i < WISP_DB_PARTITION_COUNT; i++) {
        if (partitions[i] && partitionSizes[i] >= sizeof(WispPartitionHeader)) {
            WispPartitionHeader* header = reinterpret_cast<WispPartitionHeader*>(partitions[i]);
            header->magic = WISP_DB_MAGIC;
            header->version = WISP_DB_VERSION;
            header->partitionType = static_cast<WispPartitionType>(i);
            header->size = partitionSizes[i];
            header->entryCount = 0;
            header->freeSpace = partitionSizes[i] - sizeof(WispPartitionHeader);
            header->flags = 0;
            
            // Set partition-specific flags
            if (i == WISP_DB_PARTITION_ROM) {
                header->flags |= PARTITION_FLAG_READ_ONLY;
            }
            if (config->enableCompression) {
                header->flags |= PARTITION_FLAG_COMPRESSED;
            }
            if (config->enableEncryption) {
                header->flags |= PARTITION_FLAG_ENCRYPTED;
            }
            
            // Initialize free space after header
            memset(partitions[i] + sizeof(WispPartitionHeader), 0, 
                   partitionSizes[i] - sizeof(WispPartitionHeader));
        }
    }
    
    return WISP_SUCCESS;
}

void WispPartitionedDB::cleanup() {
    // Free all allocated partitions
    for (int i = 0; i < WISP_DB_PARTITION_COUNT; i++) {
        if (partitions[i]) {
            delete[] partitions[i];
            partitions[i] = nullptr;
            partitionSizes[i] = 0;
        }
    }
    
    config = nullptr;
    initialized = false;
}

WispErrorCode WispPartitionedDB::set(uint32_t key, const void* data, size_t size, 
                                     uint8_t entryType, WispPartitionType partition, 
                                     uint8_t flags) {
    if (!initialized) {
        return WISP_ERROR_NOT_INITIALIZED;
    }
    
    if (!data || size == 0 || partition >= WISP_DB_PARTITION_COUNT) {
        return WISP_ERROR_INVALID_PARAMS;
    }
    
    if (!partitions[partition]) {
        return WISP_ERROR_PARTITION_NOT_FOUND;
    }
    
    // Check if partition is read-only
    WispPartitionHeader* header = reinterpret_cast<WispPartitionHeader*>(partitions[partition]);
    if (header->flags & PARTITION_FLAG_READ_ONLY) {
        return WISP_ERROR_READ_ONLY_PARTITION;
    }
    
    // Calculate total entry size needed
    size_t entrySize = sizeof(WispEntry) + size;
    
    // Check if we have enough space
    if (header->freeSpace < entrySize) {
        return WISP_ERROR_INSUFFICIENT_SPACE;
    }
    
    // Find insertion point (after existing entries)
    uint8_t* insertPoint = partitions[partition] + sizeof(WispPartitionHeader);
    size_t usedSpace = 0;
    
    // Skip existing entries to find end
    for (uint32_t i = 0; i < header->entryCount; i++) {
        WispEntry* entry = reinterpret_cast<WispEntry*>(insertPoint + usedSpace);
        usedSpace += sizeof(WispEntry) + entry->size;
    }
    
    // Create new entry at insertion point
    WispEntry* newEntry = reinterpret_cast<WispEntry*>(insertPoint + usedSpace);
    newEntry->key = key;
    newEntry->size = size;
    newEntry->type = entryType;
    newEntry->flags = flags;
    newEntry->timestamp = getCurrentTimestamp();
    newEntry->checksum = calculateChecksum(data, size);
    
    // Copy data after entry header
    memcpy(reinterpret_cast<uint8_t*>(newEntry) + sizeof(WispEntry), data, size);
    
    // Update partition header
    header->entryCount++;
    header->freeSpace -= entrySize;
    
    return WISP_SUCCESS;
}

WispErrorCode WispPartitionedDB::get(uint32_t key, void* buffer, size_t bufferSize, 
                                     WispPartitionType partition) {
    if (!initialized) {
        return WISP_ERROR_NOT_INITIALIZED;
    }
    
    if (!buffer || bufferSize == 0) {
        return WISP_ERROR_INVALID_PARAMS;
    }
    
    // Search specific partition if provided, otherwise search all
    if (partition < WISP_DB_PARTITION_COUNT) {
        return searchPartition(key, buffer, bufferSize, partition);
    } else {
        // Search all partitions in priority order: Runtime -> Save -> Backup -> ROM
        WispPartitionType searchOrder[] = {
            WISP_DB_PARTITION_RUNTIME,
            WISP_DB_PARTITION_SAVE,
            WISP_DB_PARTITION_BACKUP,
            WISP_DB_PARTITION_ROM
        };
        
        for (int i = 0; i < 4; i++) {
            WispErrorCode result = searchPartition(key, buffer, bufferSize, searchOrder[i]);
            if (result == WISP_SUCCESS) {
                return WISP_SUCCESS;
            }
        }
        
        return WISP_ERROR_KEY_NOT_FOUND;
    }
}

WispErrorCode WispPartitionedDB::searchPartition(uint32_t key, void* buffer, size_t bufferSize, 
                                                  WispPartitionType partition) {
    if (!partitions[partition]) {
        return WISP_ERROR_PARTITION_NOT_FOUND;
    }
    
    WispPartitionHeader* header = reinterpret_cast<WispPartitionHeader*>(partitions[partition]);
    uint8_t* searchPtr = partitions[partition] + sizeof(WispPartitionHeader);
    
    // Linear search through entries
    for (uint32_t i = 0; i < header->entryCount; i++) {
        WispEntry* entry = reinterpret_cast<WispEntry*>(searchPtr);
        
        if (entry->key == key) {
            // Found matching key
            if (bufferSize < entry->size) {
                return WISP_ERROR_BUFFER_TOO_SMALL;
            }
            
            // Copy data to buffer
            const uint8_t* entryData = reinterpret_cast<const uint8_t*>(entry) + sizeof(WispEntry);
            memcpy(buffer, entryData, entry->size);
            
            // Verify checksum if enabled
            if (calculateChecksum(entryData, entry->size) != entry->checksum) {
                return WISP_ERROR_CHECKSUM_MISMATCH;
            }
            
            return WISP_SUCCESS;
        }
        
        // Move to next entry
        searchPtr += sizeof(WispEntry) + entry->size;
    }
    
    return WISP_ERROR_KEY_NOT_FOUND;
}

WispErrorCode WispPartitionedDB::remove(uint32_t key, WispPartitionType partition) {
    if (!initialized) {
        return WISP_ERROR_NOT_INITIALIZED;
    }
    
    if (partition >= WISP_DB_PARTITION_COUNT || !partitions[partition]) {
        return WISP_ERROR_PARTITION_NOT_FOUND;
    }
    
    WispPartitionHeader* header = reinterpret_cast<WispPartitionHeader*>(partitions[partition]);
    
    // Check if partition is read-only
    if (header->flags & PARTITION_FLAG_READ_ONLY) {
        return WISP_ERROR_READ_ONLY_PARTITION;
    }
    
    uint8_t* searchPtr = partitions[partition] + sizeof(WispPartitionHeader);
    
    // Find entry to remove
    for (uint32_t i = 0; i < header->entryCount; i++) {
        WispEntry* entry = reinterpret_cast<WispEntry*>(searchPtr);
        
        if (entry->key == key) {
            // Found entry to remove
            size_t entrySize = sizeof(WispEntry) + entry->size;
            size_t remainingSize = (partitions[partition] + partitionSizes[partition]) - 
                                  (searchPtr + entrySize);
            
            // Shift remaining entries left to fill gap
            memmove(searchPtr, searchPtr + entrySize, remainingSize);
            
            // Update partition header
            header->entryCount--;
            header->freeSpace += entrySize;
            
            // Clear the freed space at the end
            memset(partitions[partition] + partitionSizes[partition] - header->freeSpace, 
                   0, entrySize);
            
            return WISP_SUCCESS;
        }
        
        // Move to next entry
        searchPtr += sizeof(WispEntry) + entry->size;
    }
    
    return WISP_ERROR_KEY_NOT_FOUND;
}

bool WispPartitionedDB::exists(uint32_t key, WispPartitionType partition) {
    uint8_t dummy;
    return get(key, &dummy, sizeof(dummy), partition) == WISP_SUCCESS;
}

WispErrorCode WispPartitionedDB::getStats(WispDBStats* stats) {
    if (!initialized || !stats) {
        return WISP_ERROR_INVALID_PARAMS;
    }
    
    memset(stats, 0, sizeof(WispDBStats));
    
    for (int i = 0; i < WISP_DB_PARTITION_COUNT; i++) {
        if (partitions[i]) {
            WispPartitionHeader* header = reinterpret_cast<WispPartitionHeader*>(partitions[i]);
            stats->totalEntries += header->entryCount;
            stats->totalSize += partitionSizes[i];
            stats->usedSize += (partitionSizes[i] - header->freeSpace);
            stats->partitionEntries[i] = header->entryCount;
            stats->partitionSizes[i] = partitionSizes[i];
            stats->partitionUsed[i] = partitionSizes[i] - header->freeSpace;
        }
    }
    
    stats->freeSize = stats->totalSize - stats->usedSize;
    stats->compressionEnabled = config->enableCompression;
    stats->encryptionEnabled = config->enableEncryption;
    
    return WISP_SUCCESS;
}

WispErrorCode WispPartitionedDB::defragment(WispPartitionType partition) {
    if (!initialized) {
        return WISP_ERROR_NOT_INITIALIZED;
    }
    
    if (partition >= WISP_DB_PARTITION_COUNT || !partitions[partition]) {
        return WISP_ERROR_PARTITION_NOT_FOUND;
    }
    
    WispPartitionHeader* header = reinterpret_cast<WispPartitionHeader*>(partitions[partition]);
    
    // Check if partition is read-only
    if (header->flags & PARTITION_FLAG_READ_ONLY) {
        return WISP_ERROR_READ_ONLY_PARTITION;
    }
    
    // For simplicity, defragmentation is already handled by remove() which compacts entries
    // In a more advanced implementation, this could sort entries by key for faster lookup
    
    return WISP_SUCCESS;
}

uint32_t WispPartitionedDB::getCurrentTimestamp() {
    // In a real implementation, this would return actual timestamp
    // For now, return milliseconds since startup
    static uint32_t counter = 0;
    return ++counter;
}

uint16_t WispPartitionedDB::calculateChecksum(const void* data, size_t size) {
    // Simple CRC16 checksum
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint16_t crc = 0xFFFF;
    
    for (size_t i = 0; i < size; i++) {
        crc ^= bytes[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

// Global database instance
WispPartitionedDB wispDB;
