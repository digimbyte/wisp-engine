// Wisp Engine Document Database System - ESP32-C6/S3 using ESP-IDF
// Document-based storage with collections and structured data
#pragma once
#include "../../system/esp32_common.h"
#include <esp_attr.h>
#include <unordered_map>
#include <map>
#include <string>
#include <vector>
#include <variant>

// Database configuration
#define DB_VERSION 1
#define DB_LP_SRAM_SIZE 16384   // ESP32-C6 LP-SRAM: 16KB

// Database partitions for different data lifecycles
enum DatabasePartition : uint8_t {
    PARTITION_PERSISTENT = 0,    // Data that survives reboots
    PARTITION_VOLATILE = 1,      // Runtime cache, cleared on restart
    PARTITION_USER_A = 2,        // Application-defined partition A
    PARTITION_USER_B = 3         // Application-defined partition B
};

// Supported field value types
using FieldValue = std::variant<
    bool,
    int32_t,
    uint32_t,
    float,
    double,
    std::string,
    std::vector<uint8_t>  // For binary data
>;

// Document structure - like a JSON object
struct Document {
    std::string id;                              // Unique document ID
    std::map<std::string, FieldValue> fields;   // Field name -> value mapping
    std::string collection;                      // Collection name (like table name)
    DatabasePartition partition;                 // Which partition
    uint32_t timestamp;                          // Last modified
    uint32_t version;                           // Document version for conflicts
    
    // Convenience methods for field access
    template<typename T>
    void setField(const std::string& fieldName, const T& value) {
        fields[fieldName] = value;
        timestamp = millis();
        version++;
    }
    
    template<typename T>
    T getField(const std::string& fieldName, const T& defaultValue = T{}) const {
        auto it = fields.find(fieldName);
        if (it != fields.end()) {
            try {
                return std::get<T>(it->second);
            } catch (const std::bad_variant_access&) {
                return defaultValue;
            }
        }
        return defaultValue;
    }
    
    bool hasField(const std::string& fieldName) const {
        return fields.find(fieldName) != fields.end();
    }
    
    void removeField(const std::string& fieldName) {
        fields.erase(fieldName);
        timestamp = millis();
        version++;
    }
};

// Query structure for finding documents
struct Query {
    std::string collection;
    std::map<std::string, FieldValue> conditions;  // Field name -> required value
    uint32_t limit = 0;  // 0 = no limit
    
    // Add condition to query
    template<typename T>
    Query& where(const std::string& fieldName, const T& value) {
        conditions[fieldName] = value;
        return *this;
    }
    
    Query& inCollection(const std::string& collectionName) {
        collection = collectionName;
        return *this;
    }
    
    Query& limitResults(uint32_t maxResults) {
        limit = maxResults;
        return *this;
    }
};

// Document database interface
class DocumentDatabase {
private:
    std::unordered_map<std::string, Document> documents;  // documentId -> document
    std::map<std::string, std::vector<std::string>> collections; // collection -> [documentIds]
    bool initialized = false;
    
    // Partition size limits (configurable)
    size_t partitionSizes[4] = {8192, 4096, 2048, 2048}; // Total: 16KB
    size_t partitionUsed[4] = {0, 0, 0, 0};
    
    bool hasSpace(DatabasePartition partition, size_t requiredBytes);
    void updatePartitionUsage(DatabasePartition partition, int64_t deltaBytes);
    size_t calculateDocumentSize(const Document& doc);
    bool matchesQuery(const Document& doc, const Query& query);

public:
    // Initialize database
    bool initialize();
    void cleanup();
    
    // Document operations
    bool insertDocument(const std::string& id, const std::string& collection, 
                       const Document& doc, DatabasePartition partition = PARTITION_PERSISTENT);
    bool updateDocument(const std::string& id, const Document& doc);
    bool deleteDocument(const std::string& id);
    bool documentExists(const std::string& id);
    Document* getDocument(const std::string& id);
    
    // Collection operations
    std::vector<std::string> getCollections();
    std::vector<Document*> getDocumentsInCollection(const std::string& collection);
    bool dropCollection(const std::string& collection);
    uint32_t getCollectionSize(const std::string& collection);
    
    // Query operations
    std::vector<Document*> find(const Query& query);
    Document* findOne(const Query& query);
    uint32_t count(const Query& query);
    
    // Convenience methods for simple document creation
    Document createDocument(const std::string& id, const std::string& collection,
                           DatabasePartition partition = PARTITION_PERSISTENT);
    
    // Memory management
    size_t getTotalUsed();
    size_t getTotalFree();
    size_t getPartitionUsed(DatabasePartition partition);
    size_t getPartitionFree(DatabasePartition partition);
    
    // Utility
    void clearPartition(DatabasePartition partition);
    std::vector<std::string> getDocumentIdsInPartition(DatabasePartition partition);
    void printStats();
    void printCollection(const std::string& collection);
};

// Global database instance
extern DocumentDatabase globalDB;
