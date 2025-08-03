// engine/save_system.cpp
#include "save_system.h"
#include <type_traits>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"

// Global save system instance
WispSaveSystem* g_SaveSystem = nullptr;

// Save result to string conversion
const char* getSaveResultString(WispSaveResult result) {
    switch (result) {
        case SAVE_SUCCESS: return "Success";
        case SAVE_ERROR_NO_STORAGE: return "No storage available";
        case SAVE_ERROR_WRITE_FAILED: return "Write failed";
        case SAVE_ERROR_READ_FAILED: return "Read failed";
        case SAVE_ERROR_INVALID_FILE: return "Invalid file format";
        case SAVE_ERROR_WRONG_APP: return "Save file belongs to different app";
        case SAVE_ERROR_VERSION_MISMATCH: return "Version mismatch";
        case SAVE_ERROR_CORRUPTED: return "Save file corrupted";
        case SAVE_ERROR_NOT_FOUND: return "Save file not found";
        case SAVE_ERROR_MEMORY_FULL: return "Memory full";
        default: return "Unknown error";
    }
}

// Initialize save system
bool WispSaveSystem::init(bool preferSDCard) {
    useSDCard = preferSDCard;
    
    if (useSDCard) {
        // ESP-IDF SD card initialization would go here
        // For now, fall back to SPIFFS
        useSDCard = false;
    }
    
    if (!useSDCard) {
        // Initialize SPIFFS using ESP-IDF
        esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true
        };
        
        esp_err_t ret = esp_vfs_spiffs_register(&conf);
        if (ret != ESP_OK) {
            DEBUG_ERROR("SAVE", "Failed to initialize SPIFFS");
            return false;
        }
    }
    
    // Create save directory if it doesn't exist
    String storage = useSDCard ? "SD" : "SPIFFS";
    
    if (!useSDCard) {
        // For SPIFFS, ensure base path exists
        struct stat st;
        String fullSaveDir = "/spiffs" + saveDirectory;
        if (stat(fullSaveDir.c_str(), &st) != 0) {
            mkdir(fullSaveDir.c_str(), 0755);
        }
    }
    
    DEBUG_INFO_STR("SAVE", "Save system initialized using " + storage);
    return true;
}

void WispSaveSystem::setAppIdentity(const WispAppIdentity& identity) {
    if (identity.uuid.empty()) {
        DEBUG_ERROR("SAVE", "App UUID cannot be empty");
        return;
    }
    
    // Validate UUID format (basic check for reverse domain notation)
    if (identity.uuid.find('.') == std::string::npos) {
        DEBUG_WARNING("SAVE", "App UUID should use reverse domain notation (e.g. com.developer.gamename)");
    }
    
    currentApp = identity;
    DEBUG_INFO_STR("SAVE", "App identity set: " + identity.uuid + " v" + identity.version);
}

void WispSaveSystem::setAutoSave(bool enabled, uint32_t intervalMs) {
    autoSave = enabled;
    autoSaveInterval = intervalMs;
    lastAutoSave = millis();
    
    DEBUG_INFO("SAVE", String("Auto-save ") + (enabled ? "enabled" : "disabled") + 
                     (enabled ? " (interval: " + std::to_string(intervalMs) + "ms)" : ""));
}

String WispSaveSystem::getSaveFilePath() const {
    // Create safe filename from UUID (replace dots and invalid chars)
    String safeUuid = currentApp.uuid;
    std::replace(safeUuid.begin(), safeUuid.end(), '.', '_');
    std::replace(safeUuid.begin(), safeUuid.end(), '/', '_');
    std::replace(safeUuid.begin(), safeUuid.end(), '\\', '_');
    
    if (useSDCard) {
        return saveDirectory + "/" + safeUuid + ".sav";
    } else {
        return "/spiffs" + saveDirectory + "/" + safeUuid + ".sav";
    }
}

String WispSaveSystem::getBackupFilePath() const {
    String savePath = getSaveFilePath();
    return savePath + ".bak";
}

uint32_t WispSaveSystem::calculateChecksum(const uint8_t* data, size_t size) const {
    // Simple CRC32 implementation
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < size; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return ~crc;
}

bool WispSaveSystem::validateSaveFile(const String& filePath, WispSaveHeader& header) const {
    FILE* file = fopen(filePath.c_str(), "rb");
    if (!file) {
        return false;
    }
    
    // Read header
    if (fread(&header, sizeof(WispSaveHeader), 1, file) != 1) {
        fclose(file);
        return false;
    }
    
    // Validate magic number
    if (header.magic != 0x57495350) {
        DEBUG_ERROR("SAVE", "Invalid magic number in save file");
        fclose(file);
        return false;
    }
    
    // Validate app identity
    if (String(header.appUuid) != currentApp.uuid) {
        DEBUG_ERROR_STR("SAVE", "Save file belongs to different app: " + std::string(header.appUuid));
        fclose(file);
        return false;
    }
    
    // Validate fingerprint
    if (header.fingerprint != currentApp.generateFingerprint()) {
        DEBUG_WARNING("SAVE", "Save file fingerprint mismatch - possible version issue");
    }
    
    // Get file size for validation
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, sizeof(WispSaveHeader), SEEK_SET);
    
    // Validate data section size
    if (header.dataSize > fileSize - sizeof(WispSaveHeader)) {
        DEBUG_ERROR("SAVE", "Save file data size mismatch");
        fclose(file);
        return false;
    }
    
    // Read and validate checksum
    uint8_t* buffer = (uint8_t*)malloc(header.dataSize);
    if (!buffer) {
        DEBUG_ERROR("SAVE", "Cannot allocate memory for save data validation");
        fclose(file);
        return false;
    }
    
    if (fread(buffer, 1, header.dataSize, file) != header.dataSize) {
        DEBUG_ERROR("SAVE", "Cannot read save data for validation");
        free(buffer);
        fclose(file);
        return false;
    }
    
    uint32_t calculatedChecksum = calculateChecksum(buffer, header.dataSize);
    free(buffer);
    fclose(file);
    
    if (calculatedChecksum != header.checksum) {
        DEBUG_ERROR("SAVE", "Save file checksum mismatch - file may be corrupted");
        return false;
    }
    
    return true;
}

WispSaveResult WispSaveSystem::save() {
    if (!isInitialized()) {
        DEBUG_ERROR("SAVE", "Save system not initialized");
        return SAVE_ERROR_NO_STORAGE;
    }
    
    String filePath = getSaveFilePath();
    
    // Create backup of existing save file
    if (hasSaveFile()) {
        createBackup();
    }
    
    WispSaveResult result = writeSaveData(filePath);
    
    if (result == SAVE_SUCCESS) {
        markAllFieldsClean();
        lastAutoSave = millis();
        DEBUG_INFO("SAVE", "Save completed successfully");
    } else {
        DEBUG_ERROR_STR("SAVE", "Save failed: " + std::string(getSaveResultString(result)));
        
        // Try to restore from backup if save failed
        if (hasSaveFile()) {
            restoreFromBackup();
        }
    }
    
    return result;
}

WispSaveResult WispSaveSystem::writeSaveData(const String& filePath) const {
    // Calculate total data size
    size_t totalDataSize = 0;
    for (const auto& pair : saveFields) {
        const WispSaveField& field = pair.second;
        totalDataSize += sizeof(uint8_t); // Type
        totalDataSize += sizeof(uint16_t); // Key length
        totalDataSize += field.key.length(); // Key string
        totalDataSize += sizeof(uint32_t); // Data size
        
        if (field.type == SAVE_TYPE_STRING) {
            String* strPtr = static_cast<String*>(field.data);
            totalDataSize += strPtr ? strPtr->length() : 0;
        } else if (field.type == SAVE_TYPE_BLOB) {
            totalDataSize += field.size;
        } else {
            totalDataSize += field.size;
        }
    }
    
    // Allocate buffer for serialized data
    uint8_t* buffer = (uint8_t*)malloc(totalDataSize);
    if (!buffer) {
        DEBUG_ERROR("SAVE", "Cannot allocate memory for save data");
        return SAVE_ERROR_MEMORY_FULL;
    }
    
    // Serialize save fields
    uint8_t* ptr = buffer;
    for (const auto& pair : saveFields) {
        const WispSaveField& field = pair.second;
        
        // Write type
        *ptr++ = (uint8_t)field.type;
        
        // Write key length and key
        uint16_t keyLen = field.key.length();
        memcpy(ptr, &keyLen, sizeof(uint16_t));
        ptr += sizeof(uint16_t);
        memcpy(ptr, field.key.c_str(), keyLen);
        ptr += keyLen;
        
        // Write data size and data
        uint32_t dataSize;
        if (field.type == SAVE_TYPE_STRING) {
            String* strPtr = static_cast<String*>(field.data);
            dataSize = strPtr ? strPtr->length() : 0;
            memcpy(ptr, &dataSize, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            if (dataSize > 0) {
                memcpy(ptr, strPtr->c_str(), dataSize);
                ptr += dataSize;
            }
        } else if (field.type == SAVE_TYPE_BLOB) {
            dataSize = field.size;
            memcpy(ptr, &dataSize, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            memcpy(ptr, field.data, dataSize);
            ptr += dataSize;
        } else {
            dataSize = field.size;
            memcpy(ptr, &dataSize, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            memcpy(ptr, field.data, dataSize);
            ptr += dataSize;
        }
    }
    
    // Create and populate header
    WispSaveHeader header;
    header.fingerprint = currentApp.generateFingerprint();
    header.saveFormatVersion = currentApp.saveFormatVersion;
    header.dataSize = totalDataSize;
    header.checksum = calculateChecksum(buffer, totalDataSize);
    header.timestamp = millis();
    strncpy(header.appUuid, currentApp.uuid.c_str(), sizeof(header.appUuid) - 1);
    strncpy(header.appVersion, currentApp.version.c_str(), sizeof(header.appVersion) - 1);
    
    // Write to file
    FILE* file = fopen(filePath.c_str(), "wb");
    if (!file) {
        free(buffer);
        DEBUG_ERROR("SAVE", "Cannot open file for writing: " + filePath);
        return SAVE_ERROR_WRITE_FAILED;
    }
    
    // Write header
    if (fwrite(&header, sizeof(WispSaveHeader), 1, file) != 1) {
        free(buffer);
        fclose(file);
        DEBUG_ERROR("SAVE", "Failed to write save header");
        return SAVE_ERROR_WRITE_FAILED;
    }
    
    // Write data
    if (fwrite(buffer, 1, totalDataSize, file) != totalDataSize) {
        free(buffer);
        fclose(file);
        DEBUG_ERROR("SAVE", "Failed to write save data");
        return SAVE_ERROR_WRITE_FAILED;
    }
    
    fclose(file);
    free(buffer);
    
    return SAVE_SUCCESS;
}

WispSaveResult WispSaveSystem::load() {
    if (!isInitialized()) {
        DEBUG_ERROR("SAVE", "Save system not initialized");
        return SAVE_ERROR_NO_STORAGE;
    }
    
    String filePath = getSaveFilePath();
    return readSaveData(filePath);
}

WispSaveResult WispSaveSystem::readSaveData(const String& filePath) {
    WispSaveHeader header;
    
    if (!validateSaveFile(filePath, header)) {
        return SAVE_ERROR_INVALID_FILE;
    }
    
    FILE* file = fopen(filePath.c_str(), "rb");
    if (!file) {
        DEBUG_ERROR("SAVE", "Cannot open save file for reading: " + filePath);
        return SAVE_ERROR_READ_FAILED;
    }
    
    // Skip header
    fseek(file, sizeof(WispSaveHeader), SEEK_SET);
    
    // Read and deserialize data
    uint8_t* buffer = (uint8_t*)malloc(header.dataSize);
    if (!buffer) {
        fclose(file);
        DEBUG_ERROR("SAVE", "Cannot allocate memory for save data");
        return SAVE_ERROR_MEMORY_FULL;
    }
    
    if (fread(buffer, 1, header.dataSize, file) != header.dataSize) {
        free(buffer);
        fclose(file);
        DEBUG_ERROR("SAVE", "Failed to read save data");
        return SAVE_ERROR_READ_FAILED;
    }
    
    fclose(file);
    
    // Deserialize fields
    uint8_t* ptr = buffer;
    uint8_t* endPtr = buffer + header.dataSize;
    
    while (ptr < endPtr) {
        // Read type
        if (ptr >= endPtr) break;
        WispSaveDataType type = (WispSaveDataType)*ptr++;
        
        // Read key
        if (ptr + sizeof(uint16_t) > endPtr) break;
        uint16_t keyLen;
        memcpy(&keyLen, ptr, sizeof(uint16_t));
        ptr += sizeof(uint16_t);
        
        if (ptr + keyLen > endPtr) break;
        String key(reinterpret_cast<const char*>(ptr), keyLen);
        ptr += keyLen;
        
        // Read data size
        if (ptr + sizeof(uint32_t) > endPtr) break;
        uint32_t dataSize;
        memcpy(&dataSize, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        
        if (ptr + dataSize > endPtr) break;
        
        // Find field in our registry
        auto it = saveFields.find(key);
        if (it != saveFields.end() && it->second.type == type) {
            WispSaveField& field = it->second;
            
            if (field.type == SAVE_TYPE_STRING) {
                String* strPtr = static_cast<String*>(field.data);
                if (strPtr) {
                    *strPtr = String(reinterpret_cast<const char*>(ptr), dataSize);
                }
            } else if (field.type == SAVE_TYPE_BLOB) {
                if (dataSize <= field.size) {
                    memcpy(field.data, ptr, dataSize);
                }
            } else {
                if (dataSize == field.size) {
                    memcpy(field.data, ptr, dataSize);
                }
            }
            
            field.isDirty = false;
        } else {
            DEBUG_WARNING("SAVE", "Unknown or type-mismatched field in save file: " + key);
        }
        
        ptr += dataSize;
    }
    
    free(buffer);
    
    DEBUG_INFO("SAVE", "Save file loaded successfully");
    return SAVE_SUCCESS;
}

bool WispSaveSystem::registerStringField(const String& key, String* stringPtr, size_t maxLength) {
    if (!stringPtr || hasField(key)) {
        DEBUG_ERROR("SAVE", "Invalid string pointer or field already exists: " + key);
        return false;
    }
    
    saveFields[key] = WispSaveField(key, SAVE_TYPE_STRING, stringPtr, maxLength);
    DEBUG_INFO("SAVE", "Registered string field: " + key);
    return true;
}

bool WispSaveSystem::registerBlobField(const String& key, void* dataPtr, size_t size) {
    if (!dataPtr || size == 0 || hasField(key)) {
        DEBUG_ERROR("SAVE", "Invalid blob parameters or field already exists: " + key);
        return false;
    }
    
    saveFields[key] = WispSaveField(key, SAVE_TYPE_BLOB, dataPtr, size);
    DEBUG_INFO_STR("SAVE", "Registered blob field: " + key + " (" + std::to_string(size) + " bytes)");
    return true;
}

String* WispSaveSystem::getStringField(const String& key) {
    auto it = saveFields.find(key);
    if (it == saveFields.end() || it->second.type != SAVE_TYPE_STRING) {
        DEBUG_WARNING("SAVE", "String field not found: " + key);
        return nullptr;
    }
    
    return static_cast<String*>(it->second.data);
}

void* WispSaveSystem::getBlobField(const String& key, size_t* outSize) {
    auto it = saveFields.find(key);
    if (it == saveFields.end() || it->second.type != SAVE_TYPE_BLOB) {
        DEBUG_WARNING("SAVE", "Blob field not found: " + key);
        return nullptr;
    }
    
    if (outSize) {
        *outSize = it->second.size;
    }
    
    return it->second.data;
}

bool WispSaveSystem::setStringField(const String& key, const String& value) {
    String* fieldPtr = getStringField(key);
    if (!fieldPtr) {
        return false;
    }
    
    *fieldPtr = value;
    saveFields[key].isDirty = true;
    
    DEBUG_INFO("SAVE", "String field updated: " + key);
    return true;
}

bool WispSaveSystem::setBlobField(const String& key, const void* data, size_t size) {
    size_t maxSize;
    void* fieldPtr = getBlobField(key, &maxSize);
    if (!fieldPtr || size > maxSize) {
        DEBUG_ERROR("SAVE", "Blob field not found or size too large: " + key);
        return false;
    }
    
    memcpy(fieldPtr, data, size);
    saveFields[key].isDirty = true;
    
    DEBUG_INFO("SAVE", "Blob field updated: " + key);
    return true;
}

WispSaveResult WispSaveSystem::reset() {
    // This would reset all fields to their default values
    // Implementation depends on whether we store default values
    for (auto& pair : saveFields) {
        pair.second.isDirty = true;
    }
    
    DEBUG_INFO("SAVE", "Save data reset to defaults");
    return SAVE_SUCCESS;
}

bool WispSaveSystem::hasSaveFile() const {
    String filePath = getSaveFilePath();
    
    struct stat st;
    return (stat(filePath.c_str(), &st) == 0);
}

bool WispSaveSystem::deleteSaveFile() {
    String filePath = getSaveFilePath();
    
    bool success = (remove(filePath.c_str()) == 0);
    
    if (success) {
        DEBUG_INFO("SAVE", "Save file deleted");
    } else {
        DEBUG_ERROR("SAVE", "Failed to delete save file");
    }
    
    return success;
}

void WispSaveSystem::update() {
    if (!autoSave || !isInitialized()) {
        return;
    }
    
    uint32_t currentTime = millis();
    if (currentTime - lastAutoSave >= autoSaveInterval) {
        // Check if any fields are dirty
        bool needsSave = false;
        for (const auto& pair : saveFields) {
            if (pair.second.isDirty) {
                needsSave = true;
                break;
            }
        }
        
        if (needsSave) {
            DEBUG_INFO("SAVE", "Auto-save triggered");
            save();
        }
        
        lastAutoSave = currentTime;
    }
}

void WispSaveSystem::createBackup() const {
    String savePath = getSaveFilePath();
    String backupPath = getBackupFilePath();
    
    // Simple file copy using standard FILE operations
    FILE* source = fopen(savePath.c_str(), "rb");
    FILE* dest = fopen(backupPath.c_str(), "wb");
    
    if (source && dest) {
        char buffer[1024];
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), source)) > 0) {
            fwrite(buffer, 1, bytesRead, dest);
        }
        DEBUG_INFO("SAVE", "Backup created");
    }
    
    if (source) fclose(source);
    if (dest) fclose(dest);
}

bool WispSaveSystem::restoreFromBackup() {
    String savePath = getSaveFilePath();
    String backupPath = getBackupFilePath();
    
    // Check if backup exists
    struct stat st;
    bool backupExists = (stat(backupPath.c_str(), &st) == 0);
    
    if (!backupExists) {
        DEBUG_ERROR("SAVE", "No backup file to restore from");
        return false;
    }
    
    // Copy backup to main save file
    FILE* source = fopen(backupPath.c_str(), "rb");
    FILE* dest = fopen(savePath.c_str(), "wb");
    
    bool success = false;
    if (source && dest) {
        char buffer[1024];
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), source)) > 0) {
            fwrite(buffer, 1, bytesRead, dest);
        }
        success = true;
        DEBUG_INFO("SAVE", "Backup restored");
    }
    
    if (source) fclose(source);
    if (dest) fclose(dest);
    
    return success;
}

// Additional utility methods
bool WispSaveSystem::hasField(const String& key) const {
    return saveFields.find(key) != saveFields.end();
}

WispSaveDataType WispSaveSystem::getFieldType(const String& key) const {
    auto it = saveFields.find(key);
    return (it != saveFields.end()) ? it->second.type : SAVE_TYPE_BOOL;
}

bool WispSaveSystem::isFieldDirty(const String& key) const {
    auto it = saveFields.find(key);
    return (it != saveFields.end()) ? it->second.isDirty : false;
}

void WispSaveSystem::markFieldClean(const String& key) {
    auto it = saveFields.find(key);
    if (it != saveFields.end()) {
        it->second.isDirty = false;
    }
}

void WispSaveSystem::markAllFieldsClean() {
    for (auto& pair : saveFields) {
        pair.second.isDirty = false;
    }
}

void WispSaveSystem::printSaveStatus() const {
    printf("=== Wisp Save System Status ===\n");
    printf("App: %s v%s\n", currentApp.uuid.c_str(), currentApp.version.c_str());
    
    printf("Storage: %s\n", useSDCard ? "SD Card" : "SPIFFS");
    
    printf("Auto-save: %s", autoSave ? "Enabled" : "Disabled");
    if (autoSave) {
        printf(" (%lu ms)", autoSaveInterval);
    }
    printf("\n");
    
    printf("Save file exists: %s\n", hasSaveFile() ? "Yes" : "No");
    
    printf("Registered fields: %zu\n", saveFields.size());
    
    printf("==============================\n");
}

void WispSaveSystem::printFieldStatus() const {
    printf("=== Save Field Status ===\n");
    
    for (const auto& pair : saveFields) {
        const WispSaveField& field = pair.second;
        printf("%s [", field.key.c_str());
        
        switch (field.type) {
            case SAVE_TYPE_BOOL: printf("BOOL"); break;
            case SAVE_TYPE_INT8: printf("INT8"); break;
            case SAVE_TYPE_UINT8: printf("UINT8"); break;
            case SAVE_TYPE_INT16: printf("INT16"); break;
            case SAVE_TYPE_UINT16: printf("UINT16"); break;
            case SAVE_TYPE_INT32: printf("INT32"); break;
            case SAVE_TYPE_UINT32: printf("UINT32"); break;
            case SAVE_TYPE_FLOAT: printf("FLOAT"); break;
            case SAVE_TYPE_STRING: printf("STRING"); break;
            case SAVE_TYPE_BLOB: printf("BLOB"); break;
        }
        
        printf("] %s (%zu bytes)\n", 
               field.isDirty ? "DIRTY" : "CLEAN",
               field.size);
    }
    
    printf("========================\n");
}

size_t WispSaveSystem::getMemoryUsage() const {
    size_t total = sizeof(WispSaveSystem);
    
    for (const auto& pair : saveFields) {
        total += pair.first.length(); // Key string
        total += sizeof(WispSaveField); // Field structure
        // Note: We don't count the actual data since it's owned by the app
    }
    
    return total;
}
