// legacy_system.cpp - Implementation of WispPartitionedDB constructor/destructor
#include "database_system.h"

// Static data initialization for RTC RAM
RTC_DATA_ATTR uint8_t WispPartitionedDB::lpSramData[WISP_DB_LP_SRAM_SIZE];

// Constructor
WispPartitionedDB::WispPartitionedDB() : 
    initialized(false),
    cacheSize(0),
    cacheCount(0),
    romPartition(nullptr),
    savePartition(nullptr),
    backupPartition(nullptr),
    runtimePartition(nullptr),
    cache(nullptr),
    romSize(0),
    saveSize(0),
    backupSize(0),
    runtimeSize(0)
{
    // Initialize partition arrays
    for (int i = 0; i < WISP_DB_PARTITION_COUNT; i++) {
        partitions[i] = nullptr;
        partitionSizes[i] = 0;
    }
    
    // Initialize config to default values
    memset(&config, 0, sizeof(config));
}

// Destructor
WispPartitionedDB::~WispPartitionedDB() {
    shutdown();
}

// Shutdown method
void WispPartitionedDB::shutdown() {
    if (!initialized) {
        return;
    }
    
    // Clean up cache if allocated
    if (cache != nullptr) {
        delete[] cache;
        cache = nullptr;
    }
    
    // Reset partition pointers (they point to stack memory, so no delete needed)
    romPartition = nullptr;
    savePartition = nullptr;
    backupPartition = nullptr;
    runtimePartition = nullptr;
    
    // Reset partition arrays
    for (int i = 0; i < WISP_DB_PARTITION_COUNT; i++) {
        partitions[i] = nullptr;
        partitionSizes[i] = 0;
    }
    
    // Reset sizes and counters
    cacheSize = 0;
    cacheCount = 0;
    romSize = 0;
    saveSize = 0;
    backupSize = 0;
    runtimeSize = 0;
    
    // Clear config
    memset(&config, 0, sizeof(config));
    
    initialized = false;
}