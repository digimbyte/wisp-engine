// Wisp Engine Document Database System - ESP32-C6/S3 using ESP-IDF
// Document-based storage with collections and structured data
#pragma once
#include "../../system/esp32_common.h"
#include <esp_attr.h>
#include <cstdint>  // For uint8_t types
#include <cstring>  // For string functions
#include <cstddef>  // For size_t
#include "esp_timer.h"  // For timing functions

// Database configuration
#define DB_VERSION 1
#define DB_LP_SRAM_SIZE (16 * 1024)  // ESP32-C6 LP-SRAM: 16KB
#define MAX_FIELD_NAME_LENGTH 64
#define MAX_STRING_VALUE_LENGTH 256
#define MAX_BINARY_VALUE_LENGTH 512
#define MAX_DOCUMENT_ID_LENGTH 64
#define MAX_COLLECTION_NAME_LENGTH 64
#define MAX_FIELDS_PER_DOCUMENT 16
#define MAX_DOCUMENTS 256
#define MAX_COLLECTIONS 32
#define MAX_DOCUMENTS_PER_COLLECTION 64

// Database partitions for different data lifecycles
enum DatabasePartition : uint8_t {
    PARTITION_PERSISTENT = 0,    // Data that survives reboots
    PARTITION_VOLATILE = 1,      // Runtime cache, cleared on restart
    PARTITION_USER_A = 2,        // Application-defined partition A
    PARTITION_USER_B = 3         // Application-defined partition B
};

// Supported field value types
enum FieldValueType : uint8_t {
    FIELD_TYPE_BOOL,
    FIELD_TYPE_INT32,
    FIELD_TYPE_UINT32,
    FIELD_TYPE_FLOAT,
    FIELD_TYPE_DOUBLE,
    FIELD_TYPE_STRING,
    FIELD_TYPE_BINARY
};

// Field value union for ESP-IDF compatibility
struct FieldValue {
    FieldValueType type;
    union {
        bool boolValue;
        int32_t int32Value;
        uint32_t uint32Value;
        float floatValue;
        double doubleValue;
        char stringValue[MAX_STRING_VALUE_LENGTH];
        struct {
            uint8_t data[MAX_BINARY_VALUE_LENGTH];
            uint16_t length;
        } binaryValue;
    };
    
    FieldValue() : type(FIELD_TYPE_BOOL), boolValue(false) {}
    
    void setBool(bool value) {
        type = FIELD_TYPE_BOOL;
        boolValue = value;
    }
    
    void setInt32(int32_t value) {
        type = FIELD_TYPE_INT32;
        int32Value = value;
    }
    
    void setUint32(uint32_t value) {
        type = FIELD_TYPE_UINT32;
        uint32Value = value;
    }
    
    void setFloat(float value) {
        type = FIELD_TYPE_FLOAT;
        floatValue = value;
    }
    
    void setDouble(double value) {
        type = FIELD_TYPE_DOUBLE;
        doubleValue = value;
    }
    
    void setString(const char* value) {
        type = FIELD_TYPE_STRING;
        strncpy(stringValue, value, MAX_STRING_VALUE_LENGTH - 1);
        stringValue[MAX_STRING_VALUE_LENGTH - 1] = '\0';
    }
    
    void setBinary(const uint8_t* data, uint16_t length) {
        type = FIELD_TYPE_BINARY;
        binaryValue.length = (length > MAX_BINARY_VALUE_LENGTH) ? MAX_BINARY_VALUE_LENGTH : length;
        memcpy(binaryValue.data, data, binaryValue.length);
    }
};

// Document field structure
struct DocumentField {
    char name[MAX_FIELD_NAME_LENGTH];
    FieldValue value;
    
    DocumentField() {
        name[0] = '\0';
    }
    
    DocumentField(const char* fieldName, const FieldValue& fieldValue) {
        strncpy(name, fieldName, MAX_FIELD_NAME_LENGTH - 1);
        name[MAX_FIELD_NAME_LENGTH - 1] = '\0';
        value = fieldValue;
    }
};

// Document structure - like a JSON object
struct Document {
    char id[MAX_DOCUMENT_ID_LENGTH];                     // Unique document ID
    DocumentField fields[MAX_FIELDS_PER_DOCUMENT];      // Field name -> value mapping
    uint8_t numFields;                                   // Number of active fields
    char collection[MAX_COLLECTION_NAME_LENGTH];        // Collection name (like table name)
    DatabasePartition partition;                         // Which partition
    uint32_t timestamp;                                  // Last modified
    uint32_t version;                                   // Document version for conflicts
    
    Document() {
        id[0] = '\0';
        collection[0] = '\0';
        numFields = 0;
        partition = PARTITION_PERSISTENT;
        timestamp = 0;
        version = 1;
    }
    
    // Convenience methods for field access
    template<typename T>
    void setField(const char* fieldName, const T& value) {
        if (numFields >= MAX_FIELDS_PER_DOCUMENT) return;
        
        // Check if field already exists
        for (uint8_t i = 0; i < numFields; i++) {
            if (strcmp(fields[i].name, fieldName) == 0) {
                // Update existing field
                _setFieldValue(fields[i].value, value);
                timestamp = esp_timer_get_time() / 1000; // ESP-IDF time in ms
                return;
            }
        }
        
        // Add new field
        strncpy(fields[numFields].name, fieldName, MAX_FIELD_NAME_LENGTH - 1);
        fields[numFields].name[MAX_FIELD_NAME_LENGTH - 1] = '\0';
        _setFieldValue(fields[numFields].value, value);
        numFields++;
        timestamp = esp_timer_get_time() / 1000; // ESP-IDF time in ms
        version++;
    }
    
    template<typename T>
    T getField(const char* fieldName, const T& defaultValue = T{}) const {
        for (uint8_t i = 0; i < numFields; i++) {
            if (strcmp(fields[i].name, fieldName) == 0) {
                return _getFieldValue<T>(fields[i].value, defaultValue);
            }
        }
        return defaultValue;
    }
    
    bool hasField(const char* fieldName) const {
        for (uint8_t i = 0; i < numFields; i++) {
            if (strcmp(fields[i].name, fieldName) == 0) {
                return true;
            }
        }
        return false;
    }
    
    void removeField(const char* fieldName) {
        for (uint8_t i = 0; i < numFields; i++) {
            if (strcmp(fields[i].name, fieldName) == 0) {
                // Shift remaining fields
                for (uint8_t j = i; j < numFields - 1; j++) {
                    fields[j] = fields[j + 1];
                }
                numFields--;
                timestamp = esp_timer_get_time() / 1000;
                version++;
                return;
            }
        }
    }
    
private:
    template<typename T>
    void _setFieldValue(FieldValue& field, const T& value);
    
    template<typename T>
    T _getFieldValue(const FieldValue& field, const T& defaultValue) const;
};

// Query structure for finding documents
struct Query {
    char collection[MAX_COLLECTION_NAME_LENGTH];
    struct QueryCondition {
        char fieldName[MAX_FIELD_NAME_LENGTH];
        FieldValue value;
    };
    QueryCondition conditions[MAX_FIELDS_PER_DOCUMENT];
    uint8_t numConditions;
    uint32_t limit; // 0 = no limit
    
    Query() : numConditions(0), limit(0) {
        collection[0] = '\0';
    }
    
    // Add condition to query
    template<typename T>
    Query& where(const char* fieldName, const T& value) {
        if (numConditions < MAX_FIELDS_PER_DOCUMENT) {
            strncpy(conditions[numConditions].fieldName, fieldName, MAX_FIELD_NAME_LENGTH - 1);
            conditions[numConditions].fieldName[MAX_FIELD_NAME_LENGTH - 1] = '\0';
            conditions[numConditions].value = FieldValue(value);
            numConditions++;
        }
        return *this;
    }
    
    Query& inCollection(const char* collectionName) {
        strncpy(collection, collectionName, MAX_COLLECTION_NAME_LENGTH - 1);
        collection[MAX_COLLECTION_NAME_LENGTH - 1] = '\0';
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
    Document documents[MAX_DOCUMENTS];
    char documentIds[MAX_DOCUMENTS][MAX_DOCUMENT_ID_LENGTH];
    uint16_t documentCount;
    
    char collections[MAX_COLLECTIONS][MAX_COLLECTION_NAME_LENGTH];
    char collectionDocuments[MAX_COLLECTIONS][MAX_DOCUMENTS_PER_COLLECTION][MAX_DOCUMENT_ID_LENGTH];
    uint8_t collectionDocCounts[MAX_COLLECTIONS];
    uint8_t collectionCount;
    
    bool initialized;
    
    // Partition size limits (configurable)
    uint32_t partitionSizes[4]; // Total: 16KB
    uint32_t partitionUsed[4];
    
    bool hasSpace(DatabasePartition partition, size_t requiredBytes);
    void updatePartitionUsage(DatabasePartition partition, int64_t deltaBytes);
    size_t calculateDocumentSize(const Document& doc);
    bool matchesQuery(const Document& doc, const Query& query);

public:
    // Initialize database
    bool initialize();
    void cleanup();
    
    // Document operations
    bool insertDocument(const char* id, const char* collection, 
                       const Document& doc, DatabasePartition partition = PARTITION_PERSISTENT);
    bool updateDocument(const char* id, const Document& doc);
    bool deleteDocument(const char* id);
    bool documentExists(const char* id);
    Document* getDocument(const char* id);
    
    // Collection operations
    uint8_t getCollections(char collections[][MAX_COLLECTION_NAME_LENGTH], uint8_t maxCollections);
    uint8_t getDocumentsInCollection(const char* collection, Document** docs, uint8_t maxDocs);
    bool dropCollection(const char* collection);
    uint32_t getCollectionSize(const char* collection);
    
    // Query operations
    uint8_t find(const Query& query, Document** results, uint8_t maxResults);
    Document* findOne(const Query& query);
    uint32_t count(const Query& query);
    
    // Convenience methods for simple document creation
    Document createDocument(const char* id, const char* collection,
                           DatabasePartition partition = PARTITION_PERSISTENT);
    
    // Memory management
    size_t getTotalUsed();
    size_t getTotalFree();
    size_t getPartitionUsed(DatabasePartition partition);
    size_t getPartitionFree(DatabasePartition partition);
    
    // Utility
    void clearPartition(DatabasePartition partition);
    uint8_t getDocumentIdsInPartition(DatabasePartition partition, char ids[][MAX_DOCUMENT_ID_LENGTH], uint8_t maxIds);
    void printStats();
    void printCollection(const char* collection);
};

// Global database instance
extern DocumentDatabase globalDB;
