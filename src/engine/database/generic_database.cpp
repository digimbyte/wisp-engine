// Wisp Engine Document Database Implementation
#include "generic_database.h"

// Global instance
DocumentDatabase globalDB;

bool DocumentDatabase::initialize() {
    if (initialized) return true;
    
    // Clear all partitions
    for (int i = 0; i < 4; i++) {
        partitionUsed[i] = 0;
    }
    documents.clear();
    collections.clear();
    
    initialized = true;
    ESP_LOGI("DB", "Document database initialized - 16KB LP-SRAM");
    return true;
}

void DocumentDatabase::cleanup() {
    documents.clear();
    collections.clear();
    initialized = false;
}

size_t DocumentDatabase::calculateDocumentSize(const Document& doc) {
    size_t size = sizeof(Document);
    size += doc.id.length();
    size += doc.collection.length();
    
    for (const auto& field : doc.fields) {
        size += field.first.length(); // field name
        
        // Calculate value size based on type
        std::visit([&size](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::string>) {
                size += value.length();
            } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                size += value.size();
            } else {
                size += sizeof(T);
            }
        }, field.second);
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

bool DocumentDatabase::insertDocument(const std::string& id, const std::string& collection, 
                                     const Document& doc, DatabasePartition partition) {
    if (!initialized) return false;
    
    // Check if document already exists
    if (documents.find(id) != documents.end()) {
        ESP_LOGW("DB", "Document %s already exists", id.c_str());
        return false;
    }
    
    size_t docSize = calculateDocumentSize(doc);
    if (!hasSpace(partition, docSize)) {
        ESP_LOGE("DB", "Not enough space in partition %d for document %s", partition, id.c_str());
        return false;
    }
    
    // Create new document
    Document newDoc = doc;
    newDoc.id = id;
    newDoc.collection = collection;
    newDoc.partition = partition;
    newDoc.timestamp = millis();
    newDoc.version = 1;
    
    // Store document
    documents[id] = newDoc;
    collections[collection].push_back(id);
    updatePartitionUsage(partition, docSize);
    
    ESP_LOGI("DB", "Inserted document %s into collection %s", id.c_str(), collection.c_str());
    return true;
}

bool DocumentDatabase::updateDocument(const std::string& id, const Document& doc) {
    auto it = documents.find(id);
    if (it == documents.end()) return false;
    
    size_t oldSize = calculateDocumentSize(it->second);
    size_t newSize = calculateDocumentSize(doc);
    int64_t sizeDelta = newSize - oldSize;
    
    if (sizeDelta > 0 && !hasSpace(it->second.partition, sizeDelta)) {
        ESP_LOGE("DB", "Not enough space to update document %s", id.c_str());
        return false;
    }
    
    // Preserve document metadata
    Document updatedDoc = doc;
    updatedDoc.id = it->second.id;
    updatedDoc.collection = it->second.collection;
    updatedDoc.partition = it->second.partition;
    updatedDoc.timestamp = millis();
    updatedDoc.version = it->second.version + 1;
    
    it->second = updatedDoc;
    updatePartitionUsage(it->second.partition, sizeDelta);
    
    return true;
}

bool DocumentDatabase::deleteDocument(const std::string& id) {
    auto it = documents.find(id);
    if (it == documents.end()) return false;
    
    // Remove from collection index
    auto& collectionDocs = collections[it->second.collection];
    collectionDocs.erase(std::remove(collectionDocs.begin(), collectionDocs.end(), id), 
                        collectionDocs.end());
    
    // Update partition usage
    size_t docSize = calculateDocumentSize(it->second);
    updatePartitionUsage(it->second.partition, -(int64_t)docSize);
    
    documents.erase(it);
    return true;
}

bool DocumentDatabase::documentExists(const std::string& id) {
    return documents.find(id) != documents.end();
}

Document* DocumentDatabase::getDocument(const std::string& id) {
    auto it = documents.find(id);
    return (it != documents.end()) ? &it->second : nullptr;
}

std::vector<std::string> DocumentDatabase::getCollections() {
    std::vector<std::string> collectionNames;
    for (const auto& collection : collections) {
        if (!collection.second.empty()) {
            collectionNames.push_back(collection.first);
        }
    }
    return collectionNames;
}

std::vector<Document*> DocumentDatabase::getDocumentsInCollection(const std::string& collection) {
    std::vector<Document*> docs;
    auto it = collections.find(collection);
    if (it != collections.end()) {
        for (const std::string& docId : it->second) {
            auto docIt = documents.find(docId);
            if (docIt != documents.end()) {
                docs.push_back(&docIt->second);
            }
        }
    }
    return docs;
}

bool DocumentDatabase::matchesQuery(const Document& doc, const Query& query) {
    // Check collection
    if (!query.collection.empty() && doc.collection != query.collection) {
        return false;
    }
    
    // Check all conditions
    for (const auto& condition : query.conditions) {
        auto fieldIt = doc.fields.find(condition.first);
        if (fieldIt == doc.fields.end() || fieldIt->second != condition.second) {
            return false;
        }
    }
    
    return true;
}

std::vector<Document*> DocumentDatabase::find(const Query& query) {
    std::vector<Document*> results;
    
    for (auto& docPair : documents) {
        if (matchesQuery(docPair.second, query)) {
            results.push_back(&docPair.second);
            if (query.limit > 0 && results.size() >= query.limit) {
                break;
            }
        }
    }
    
    return results;
}

Document* DocumentDatabase::findOne(const Query& query) {
    for (auto& docPair : documents) {
        if (matchesQuery(docPair.second, query)) {
            return &docPair.second;
        }
    }
    return nullptr;
}

uint32_t DocumentDatabase::count(const Query& query) {
    uint32_t count = 0;
    for (const auto& docPair : documents) {
        if (matchesQuery(docPair.second, query)) {
            count++;
        }
    }
    return count;
}

Document DocumentDatabase::createDocument(const std::string& id, const std::string& collection,
                                        DatabasePartition partition) {
    Document doc;
    doc.id = id;
    doc.collection = collection;
    doc.partition = partition;
    doc.timestamp = millis();
    doc.version = 0;
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
    ESP_LOGI("DB", "=== Document Database Statistics ===");
    ESP_LOGI("DB", "Total: %zu/%d bytes (%.1f%% used)", 
             getTotalUsed(), DB_LP_SRAM_SIZE, 
             (float)getTotalUsed() / DB_LP_SRAM_SIZE * 100.0f);
    
    const char* partitionNames[] = {"Persistent", "Volatile", "User A", "User B"};
    for (int i = 0; i < 4; i++) {
        ESP_LOGI("DB", "%s: %zu/%zu bytes", partitionNames[i], 
                 partitionUsed[i], partitionSizes[i]);
    }
    
    ESP_LOGI("DB", "Total documents: %zu", documents.size());
    ESP_LOGI("DB", "Collections: %zu", collections.size());
    
    for (const auto& collection : collections) {
        if (!collection.second.empty()) {
            ESP_LOGI("DB", "  - %s: %zu documents", collection.first.c_str(), collection.second.size());
        }
    }
}

void DocumentDatabase::printCollection(const std::string& collection) {
    ESP_LOGI("DB", "=== Collection: %s ===", collection.c_str());
    auto docs = getDocumentsInCollection(collection);
    for (const auto* doc : docs) {
        ESP_LOGI("DB", "Document %s (v%u):", doc->id.c_str(), doc->version);
        for (const auto& field : doc->fields) {
            std::string valueStr;
            std::visit([&valueStr](const auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    valueStr = value;
                } else if constexpr (std::is_same_v<T, bool>) {
                    valueStr = value ? "true" : "false";
                } else if constexpr (std::is_arithmetic_v<T>) {
                    valueStr = std::to_string(value);
                } else {
                    valueStr = "[binary data]";
                }
            }, field.second);
            ESP_LOGI("DB", "  %s: %s", field.first.c_str(), valueStr.c_str());
        }
    }
}
