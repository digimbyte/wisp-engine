// Wisp Engine Document Database Implementation
#include "generic_database.h"
#include "../core/debug.h"
#include "esp_log.h"

using namespace WispEngine::Core;

// Partition names for debugging
static const char* partitionNames[] = {
    "Persistent", "Volatile", "User A", "User B"
};

// Global instance
DocumentDatabase globalDB;

bool DocumentDatabase::initialize() {
    if (initialized) return true;
    
    // Clear all partitions
    for (int i = 0; i < 4; i++) {
        partitionUsed[i] = 0;
    }
    
    // Clear document storage
    documentCount = 0;
    collectionCount = 0;
    
    // Clear collection document counts
    for (int i = 0; i < MAX_COLLECTIONS; i++) {
        collectionDocCounts[i] = 0;
    }
    
    initialized = true;
    WISP_DEBUG_INFO("DB", "Document database initialized - 16KB LP-SRAM");
    return true;
}

void DocumentDatabase::cleanup() {
    documentCount = 0;
    collectionCount = 0;
    
    // Clear collection document counts
    for (int i = 0; i < MAX_COLLECTIONS; i++) {
        collectionDocCounts[i] = 0;
    }
    
    initialized = false;
}

size_t DocumentDatabase::calculateDocumentSize(const Document& doc) {
    size_t size = sizeof(Document);
    size += strlen(doc.id);
    size += strlen(doc.collection);
    
    for (uint8_t i = 0; i < doc.numFields; i++) {
        const DocumentField& field = doc.fields[i];
        size += strlen(field.name); // field name
        
        // Calculate value size based on type
        switch (field.value.type) {
            case FIELD_TYPE_BOOL:
            case FIELD_TYPE_INT32:
            case FIELD_TYPE_UINT32:
            case FIELD_TYPE_FLOAT:
            case FIELD_TYPE_DOUBLE:
                size += sizeof(field.value);
                break;
            case FIELD_TYPE_STRING:
                size += strlen(field.value.stringValue);
                break;
            case FIELD_TYPE_BINARY:
                size += field.value.binaryValue.length;
                break;
        }
    }
    
    return size;
}

bool DocumentDatabase::hasSpace(DatabasePartition partition, size_t requiredBytes) {
    if (partition >= 4) return false;
    return (partitionUsed[partition] + requiredBytes) <= partitionSizes[partition];
}

void DocumentDatabase::updatePartitionUsage(DatabasePartition partition, int64_t deltaBytes) {
    if (partition >= 4) return;
    partitionUsed[partition] = (partitionUsed[partition] + deltaBytes);
}

bool DocumentDatabase::insertDocument(const char* id, const char* collection, 
                                     const Document& doc, DatabasePartition partition) {
    if (!initialized) return false;
    
    // Check if document already exists
    for (uint16_t i = 0; i < documentCount; i++) {
        if (strcmp(documentIds[i], id) == 0) {
            WISP_DEBUG_WARNING("DB", "Document already exists");
            return false;
        }
    }
    
    // Check if we have space for more documents
    if (documentCount >= MAX_DOCUMENTS) {
        WISP_DEBUG_ERROR("DB", "Maximum documents exceeded");
        return false;
    }
    
    size_t docSize = calculateDocumentSize(doc);
    if (!hasSpace(partition, docSize)) {
        WISP_DEBUG_ERROR("DB", "Not enough space in partition for document");
        return false;
    }
    
    // Create new document
    Document newDoc = doc;
    strncpy(newDoc.id, id, MAX_DOCUMENT_ID_LENGTH - 1);
    newDoc.id[MAX_DOCUMENT_ID_LENGTH - 1] = '\0';
    strncpy(newDoc.collection, collection, MAX_COLLECTION_NAME_LENGTH - 1);
    newDoc.collection[MAX_COLLECTION_NAME_LENGTH - 1] = '\0';
    newDoc.partition = partition;
    newDoc.timestamp = esp_timer_get_time() / 1000;
    newDoc.version = 1;
    
    // Store document
    documents[documentCount] = newDoc;
    strncpy(documentIds[documentCount], id, MAX_DOCUMENT_ID_LENGTH - 1);
    documentIds[documentCount][MAX_DOCUMENT_ID_LENGTH - 1] = '\0';
    documentCount++;
    
    // Add to collection index
    int collectionIndex = -1;
    for (uint8_t i = 0; i < collectionCount; i++) {
        if (strcmp(collections[i], collection) == 0) {
            collectionIndex = i;
            break;
        }
    }
    
    // Create new collection if it doesn't exist
    if (collectionIndex == -1) {
        if (collectionCount >= MAX_COLLECTIONS) {
            WISP_DEBUG_ERROR("DB", "Maximum collections exceeded");
            return false;
        }
        collectionIndex = collectionCount;
        strncpy(collections[collectionIndex], collection, MAX_COLLECTION_NAME_LENGTH - 1);
        collections[collectionIndex][MAX_COLLECTION_NAME_LENGTH - 1] = '\0';
        collectionDocCounts[collectionIndex] = 0;
        collectionCount++;
    }
    
    // Add document to collection
    if (collectionDocCounts[collectionIndex] >= MAX_DOCUMENTS_PER_COLLECTION) {
        WISP_DEBUG_ERROR("DB", "Collection document limit exceeded");
        return false;
    }
    
    strncpy(collectionDocuments[collectionIndex][collectionDocCounts[collectionIndex]], id, MAX_DOCUMENT_ID_LENGTH - 1);
    collectionDocuments[collectionIndex][collectionDocCounts[collectionIndex]][MAX_DOCUMENT_ID_LENGTH - 1] = '\0';
    collectionDocCounts[collectionIndex]++;
    
    updatePartitionUsage(partition, docSize);
    
    WISP_DEBUG_INFO("DB", "Inserted document into collection");
    return true;
}

bool DocumentDatabase::updateDocument(const char* id, const Document& doc) {
    // Find the document
    int docIndex = -1;
    for (uint16_t i = 0; i < documentCount; i++) {
        if (strcmp(documentIds[i], id) == 0) {
            docIndex = i;
            break;
        }
    }
    
    if (docIndex == -1) return false;
    
    size_t oldSize = calculateDocumentSize(documents[docIndex]);
    size_t newSize = calculateDocumentSize(doc);
    int64_t sizeDelta = newSize - oldSize;
    
    if (sizeDelta > 0 && !hasSpace(documents[docIndex].partition, sizeDelta)) {
        WISP_DEBUG_ERROR("DB", "Not enough space to update document");
        return false;
    }
    
    // Preserve document metadata
    Document updatedDoc = doc;
    strncpy(updatedDoc.id, documents[docIndex].id, MAX_DOCUMENT_ID_LENGTH - 1);
    updatedDoc.id[MAX_DOCUMENT_ID_LENGTH - 1] = '\0';
    strncpy(updatedDoc.collection, documents[docIndex].collection, MAX_COLLECTION_NAME_LENGTH - 1);
    updatedDoc.collection[MAX_COLLECTION_NAME_LENGTH - 1] = '\0';
    updatedDoc.partition = documents[docIndex].partition;
    updatedDoc.timestamp = esp_timer_get_time() / 1000;
    updatedDoc.version = documents[docIndex].version + 1;
    
    documents[docIndex] = updatedDoc;
    updatePartitionUsage(documents[docIndex].partition, sizeDelta);
    
    return true;
}

bool DocumentDatabase::deleteDocument(const char* id) {
    // Find the document
    int docIndex = -1;
    for (uint16_t i = 0; i < documentCount; i++) {
        if (strcmp(documentIds[i], id) == 0) {
            docIndex = i;
            break;
        }
    }
    
    if (docIndex == -1) return false;
    
    // Find the collection index
    int collectionIndex = -1;
    for (uint8_t i = 0; i < collectionCount; i++) {
        if (strcmp(collections[i], documents[docIndex].collection) == 0) {
            collectionIndex = i;
            break;
        }
    }
    
    // Remove from collection index
    if (collectionIndex >= 0) {
        for (uint8_t i = 0; i < collectionDocCounts[collectionIndex]; i++) {
            if (strcmp(collectionDocuments[collectionIndex][i], id) == 0) {
                // Shift remaining documents down
                for (uint8_t j = i; j < collectionDocCounts[collectionIndex] - 1; j++) {
                    strcpy(collectionDocuments[collectionIndex][j], collectionDocuments[collectionIndex][j + 1]);
                }
                collectionDocCounts[collectionIndex]--;
                break;
            }
        }
    }
    
    // Update partition usage
    size_t docSize = calculateDocumentSize(documents[docIndex]);
    updatePartitionUsage(documents[docIndex].partition, -(int64_t)docSize);
    
    // Remove document by shifting array elements
    for (uint16_t i = docIndex; i < documentCount - 1; i++) {
        documents[i] = documents[i + 1];
        strcpy(documentIds[i], documentIds[i + 1]);
    }
    documentCount--;
    
    return true;
}

bool DocumentDatabase::documentExists(const char* id) {
    for (uint16_t i = 0; i < documentCount; i++) {
        if (strcmp(documentIds[i], id) == 0) {
            return true;
        }
    }
    return false;
}

Document* DocumentDatabase::getDocument(const char* id) {
    for (uint16_t i = 0; i < documentCount; i++) {
        if (strcmp(documentIds[i], id) == 0) {
            return &documents[i];
        }
    }
    return nullptr;
}

uint8_t DocumentDatabase::getCollections(char collections[][MAX_COLLECTION_NAME_LENGTH], uint8_t maxCollections) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < collectionCount && count < maxCollections; i++) {
        strncpy(collections[count], this->collections[i], MAX_COLLECTION_NAME_LENGTH - 1);
        collections[count][MAX_COLLECTION_NAME_LENGTH - 1] = '\0';
        count++;
    }
    return count;
}

uint8_t DocumentDatabase::getDocumentsInCollection(const char* collection, Document** docs, uint8_t maxDocs) {
    uint8_t count = 0;
    
    // Find the collection index
    int collectionIndex = -1;
    for (uint8_t i = 0; i < collectionCount; i++) {
        if (strcmp(collections[i], collection) == 0) {
            collectionIndex = i;
            break;
        }
    }
    
    if (collectionIndex == -1) return 0;
    
    // Get documents from this collection
    for (uint8_t i = 0; i < collectionDocCounts[collectionIndex] && count < maxDocs; i++) {
        const char* docId = collectionDocuments[collectionIndex][i];
        
        // Find the document by ID
        for (uint16_t j = 0; j < documentCount; j++) {
            if (strcmp(documentIds[j], docId) == 0) {
                docs[count] = &documents[j];
                count++;
                break;
            }
        }
    }
    
    return count;
}

bool DocumentDatabase::matchesQuery(const Document& doc, const Query& query) {
    // Check collection
    if (query.collection[0] != '\0' && strcmp(doc.collection, query.collection) != 0) {
        return false;
    }
    
    // Check all conditions
    for (uint8_t i = 0; i < query.numConditions; i++) {
        const Query::QueryCondition& condition = query.conditions[i];
        
        // Find the field in the document
        bool fieldFound = false;
        for (uint8_t j = 0; j < doc.numFields; j++) {
            if (strcmp(doc.fields[j].name, condition.fieldName) == 0) {
                // TODO: Add proper field value comparison based on type
                // For now, just check if field exists
                fieldFound = true;
                break;
            }
        }
        
        if (!fieldFound) {
            return false;
        }
    }
    
    return true;
}

uint8_t DocumentDatabase::find(const Query& query, Document** results, uint8_t maxResults) {
    uint8_t count = 0;
    
    for (uint16_t i = 0; i < documentCount && count < maxResults; i++) {
        if (matchesQuery(documents[i], query)) {
            results[count] = &documents[i];
            count++;
            if (query.limit > 0 && count >= query.limit) {
                break;
            }
        }
    }
    
    return count;
}

Document* DocumentDatabase::findOne(const Query& query) {
    for (uint16_t i = 0; i < documentCount; i++) {
        if (matchesQuery(documents[i], query)) {
            return &documents[i];
        }
    }
    return nullptr;
}

uint32_t DocumentDatabase::count(const Query& query) {
    uint32_t count = 0;
    for (uint16_t i = 0; i < documentCount; i++) {
        if (matchesQuery(documents[i], query)) {
            count++;
        }
    }
    return count;
}

Document DocumentDatabase::createDocument(const char* id, const char* collection,
                                        DatabasePartition partition) {
    Document doc;
    strncpy(doc.id, id, MAX_DOCUMENT_ID_LENGTH - 1);
    doc.id[MAX_DOCUMENT_ID_LENGTH - 1] = '\0';
    strncpy(doc.collection, collection, MAX_COLLECTION_NAME_LENGTH - 1);
    doc.collection[MAX_COLLECTION_NAME_LENGTH - 1] = '\0';
    doc.partition = partition;
    doc.timestamp = esp_timer_get_time() / 1000;
    doc.version = 0;
    doc.numFields = 0;
    return doc;
}

size_t DocumentDatabase::getTotalUsed() {
    size_t total = 0;
    for (int i = 0; i < 4; i++) {
        total += partitionUsed[i];
    }
    return total;
}

size_t DocumentDatabase::getTotalFree() {
    return DB_LP_SRAM_SIZE - getTotalUsed();
}

size_t DocumentDatabase::getPartitionUsed(DatabasePartition partition) {
    if (partition >= 4) return 0;
    return partitionUsed[partition];
}

size_t DocumentDatabase::getPartitionFree(DatabasePartition partition) {
    if (partition >= 4) return 0;
    return partitionSizes[partition] - partitionUsed[partition];
}

void DocumentDatabase::printStats() {
    WISP_DEBUG_INFO("DB", "=== Document Database Statistics ===");
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Total: %u/%d bytes (%.1f%% used)", 
             getTotalUsed(), DB_LP_SRAM_SIZE, 
             (float)getTotalUsed() / DB_LP_SRAM_SIZE * 100.0f);
    WISP_DEBUG_INFO("DB", buffer);
    
    for (int i = 0; i < 4; i++) {
        snprintf(buffer, sizeof(buffer), "%s: %lu/%lu bytes", partitionNames[i], 
                 (unsigned long)partitionUsed[i], (unsigned long)partitionSizes[i]);
        WISP_DEBUG_INFO("DB", buffer);
    }
    
    snprintf(buffer, sizeof(buffer), "Total documents: %u", documentCount);
    WISP_DEBUG_INFO("DB", buffer);
    snprintf(buffer, sizeof(buffer), "Collections: %u", collectionCount);
    WISP_DEBUG_INFO("DB", buffer);
    
    for (uint8_t i = 0; i < collectionCount; i++) {
        snprintf(buffer, sizeof(buffer), "  - %s: %u documents", collections[i], collectionDocCounts[i]);
        WISP_DEBUG_INFO("DB", buffer);
    }
}

void DocumentDatabase::printCollection(const char* collection) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "=== Collection: %s ===", collection);
    WISP_DEBUG_INFO("DB", buffer);
    
    Document* docs[MAX_DOCUMENTS_PER_COLLECTION];
    uint8_t docCount = getDocumentsInCollection(collection, docs, MAX_DOCUMENTS_PER_COLLECTION);
    
    for (uint8_t i = 0; i < docCount; i++) {
        const Document* doc = docs[i];
        snprintf(buffer, sizeof(buffer), "Document %s (v%lu):", doc->id, (unsigned long)doc->version);
        WISP_DEBUG_INFO("DB", buffer);
        for (uint8_t j = 0; j < doc->numFields; j++) {
            const DocumentField& field = doc->fields[j];
            // Print field name and basic info
            snprintf(buffer, sizeof(buffer), "  %s: [type %d]", field.name, field.value.type);
            WISP_DEBUG_INFO("DB", buffer);
        }
    }
}
