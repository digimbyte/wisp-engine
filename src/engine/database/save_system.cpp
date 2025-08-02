// engine/save_system.cpp
#include "save_system.h"
#include <type_traits>

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
    useSDCard = preferSDCard && SD.begin();
    
    if (!useSDCard && !SPIFFS.begin()) {
        WISP_DEBUG_ERROR("SAVE", "No storage system available");
        return false;
    }
    
    // Create save directory if it doesn't exist
    String storage = useSDCard ? "SD" : "SPIFFS";
    
    if (useSDCard) {
        if (!SD.exists(saveDirectory)) {
            SD.mkdir(saveDirectory);
        }
    } else {
        // SPIFFS doesn't need mkdir, directories are virtual
    }
    
    WISP_DEBUG_INFO("SAVE", "Save system initialized using " + storage);
    return true;
}

void WispSaveSystem::setAppIdentity(const WispAppIdentity& identity) {
    if (identity.uuid.isEmpty()) {
        WISP_DEBUG_ERROR("SAVE", "App UUID cannot be empty");
        return;
    }
    
    // Validate UUID format (basic check for reverse domain notation)
    if (identity.uuid.indexOf('.') < 0) {
        WISP_DEBUG_WARNING("SAVE", "App UUID should use reverse domain notation (e.g. com.developer.gamename)");
    }
    
    currentApp = identity;
    WISP_DEBUG_INFO("SAVE", "App identity set: " + identity.uuid + " v" + identity.version);
}

void WispSaveSystem::setAutoSave(bool enabled, uint32_t intervalMs) {
    autoSave = enabled;
    autoSaveInterval = intervalMs;
    lastAutoSave = millis();
    
    WISP_DEBUG_INFO("SAVE", String("Auto-save ") + (enabled ? "enabled" : "disabled") + 
                     (enabled ? " (interval: " + String(intervalMs) + "ms)" : ""));
}

String WispSaveSystem::getSaveFilePath() const {
    // Create safe filename from UUID (replace dots and invalid chars)
    String safeUuid = currentApp.uuid;
    safeUuid.replace(".", "_");
    safeUuid.replace("/", "_");
    safeUuid.replace("\\", "_");
    
    return saveDirectory + "/" + safeUuid + ".sav";
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
    File file;
    
    if (useSDCard) {
        file = SD.open(filePath, FILE_READ);
    } else {
        file = SPIFFS.open(filePath, FILE_READ);
    }
    
    if (!file) {
        return false;
    }
    
    // Read header
    if (file.readBytes((char*)&header, sizeof(WispSaveHeader)) != sizeof(WispSaveHeader)) {
        file.close();
        return false;
    }
    
    // Validate magic number
    if (header.magic != 0x57495350) {
        WISP_DEBUG_ERROR("SAVE", "Invalid magic number in save file");
        file.close();
        return false;
    }
    
    // Validate app identity
    if (String(header.appUuid) != currentApp.uuid) {
        WISP_DEBUG_ERROR("SAVE", "Save file belongs to different app: " + String(header.appUuid));
        file.close();
        return false;
    }
    
    // Validate fingerprint
    if (header.fingerprint != currentApp.generateFingerprint()) {
        WISP_DEBUG_WARNING("SAVE", "Save file fingerprint mismatch - possible version issue");
    }
    
    // Validate data section size
    if (header.dataSize > file.size() - sizeof(WispSaveHeader)) {
        WISP_DEBUG_ERROR("SAVE", "Save file data size mismatch");
        file.close();
        return false;
    }
    
    // Read and validate checksum
    uint8_t* buffer = (uint8_t*)malloc(header.dataSize);
    if (!buffer) {
        WISP_DEBUG_ERROR("SAVE", "Cannot allocate memory for save data validation");
        file.close();
        return false;
    }
    
    if (file.readBytes((char*)buffer, header.dataSize) != header.dataSize) {
        WISP_DEBUG_ERROR("SAVE", "Cannot read save data for validation");
        free(buffer);
        file.close();
        return false;
    }
    
    uint32_t calculatedChecksum = calculateChecksum(buffer, header.dataSize);
    free(buffer);
    file.close();
    
    if (calculatedChecksum != header.checksum) {
        WISP_DEBUG_ERROR("SAVE", "Save file checksum mismatch - file may be corrupted");
        return false;
    }
    
    return true;
}

WispSaveResult WispSaveSystem::save() {
    if (!isInitialized()) {
        WISP_DEBUG_ERROR("SAVE", "Save system not initialized");
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
        WISP_DEBUG_INFO("SAVE", "Save completed successfully");
    } else {
        WISP_DEBUG_ERROR("SAVE", "Save failed: " + String(getSaveResultString(result)));
        
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
        WISP_DEBUG_ERROR("SAVE", "Cannot allocate memory for save data");
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
    File file;
    if (useSDCard) {
        file = SD.open(filePath, FILE_WRITE);
    } else {
        file = SPIFFS.open(filePath, FILE_WRITE);
    }
    
    if (!file) {
        free(buffer);
        WISP_DEBUG_ERROR("SAVE", "Cannot open file for writing: " + filePath);
        return SAVE_ERROR_WRITE_FAILED;
    }
    
    // Write header
    if (file.write((uint8_t*)&header, sizeof(WispSaveHeader)) != sizeof(WispSaveHeader)) {
        free(buffer);
        file.close();
        WISP_DEBUG_ERROR("SAVE", "Failed to write save header");
        return SAVE_ERROR_WRITE_FAILED;
    }
    
    // Write data
    if (file.write(buffer, totalDataSize) != totalDataSize) {
        free(buffer);
        file.close();
        WISP_DEBUG_ERROR("SAVE", "Failed to write save data");
        return SAVE_ERROR_WRITE_FAILED;
    }
    
    file.close();
    free(buffer);
    
    return SAVE_SUCCESS;
}

WispSaveResult WispSaveSystem::load() {
    if (!isInitialized()) {
        WISP_DEBUG_ERROR("SAVE", "Save system not initialized");
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
    
    File file;
    if (useSDCard) {
        file = SD.open(filePath, FILE_READ);
    } else {
        file = SPIFFS.open(filePath, FILE_READ);
    }
    
    if (!file) {
        WISP_DEBUG_ERROR("SAVE", "Cannot open save file for reading: " + filePath);
        return SAVE_ERROR_READ_FAILED;
    }
    
    // Skip header
    file.seek(sizeof(WispSaveHeader));
    
    // Read and deserialize data
    uint8_t* buffer = (uint8_t*)malloc(header.dataSize);
    if (!buffer) {
        file.close();
        WISP_DEBUG_ERROR("SAVE", "Cannot allocate memory for save data");
        return SAVE_ERROR_MEMORY_FULL;
    }
    
    if (file.readBytes((char*)buffer, header.dataSize) != header.dataSize) {
        free(buffer);
        file.close();
        WISP_DEBUG_ERROR("SAVE", "Failed to read save data");
        return SAVE_ERROR_READ_FAILED;
    }
    
    file.close();
    
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
        String key = String((char*)ptr, keyLen);
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
                    *strPtr = String((char*)ptr, dataSize);
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
            WISP_DEBUG_WARNING("SAVE", "Unknown or type-mismatched field in save file: " + key);
        }
        
        ptr += dataSize;
    }
    
    free(buffer);
    
    WISP_DEBUG_INFO("SAVE", "Save file loaded successfully");
    return SAVE_SUCCESS;
}

bool WispSaveSystem::registerStringField(const String& key, String* stringPtr, size_t maxLength) {
    if (!stringPtr || hasField(key)) {
        WISP_DEBUG_ERROR("SAVE", "Invalid string pointer or field already exists: " + key);
        return false;
    }
    
    saveFields[key] = WispSaveField(key, SAVE_TYPE_STRING, stringPtr, maxLength);
    WISP_DEBUG_INFO("SAVE", "Registered string field: " + key);
    return true;
}

bool WispSaveSystem::registerBlobField(const String& key, void* dataPtr, size_t size) {
    if (!dataPtr || size == 0 || hasField(key)) {
        WISP_DEBUG_ERROR("SAVE", "Invalid blob parameters or field already exists: " + key);
        return false;
    }
    
    saveFields[key] = WispSaveField(key, SAVE_TYPE_BLOB, dataPtr, size);
    WISP_DEBUG_INFO("SAVE", "Registered blob field: " + key + " (" + String(size) + " bytes)");
    return true;
}

String* WispSaveSystem::getStringField(const String& key) {
    auto it = saveFields.find(key);
    if (it == saveFields.end() || it->second.type != SAVE_TYPE_STRING) {
        WISP_DEBUG_WARNING("SAVE", "String field not found: " + key);
        return nullptr;
    }
    
    return static_cast<String*>(it->second.data);
}

void* WispSaveSystem::getBlobField(const String& key, size_t* outSize) {
    auto it = saveFields.find(key);
    if (it == saveFields.end() || it->second.type != SAVE_TYPE_BLOB) {
        WISP_DEBUG_WARNING("SAVE", "Blob field not found: " + key);
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
    
    WISP_DEBUG_INFO("SAVE", "String field updated: " + key);
    return true;
}

bool WispSaveSystem::setBlobField(const String& key, const void* data, size_t size) {
    size_t maxSize;
    void* fieldPtr = getBlobField(key, &maxSize);
    if (!fieldPtr || size > maxSize) {
        WISP_DEBUG_ERROR("SAVE", "Blob field not found or size too large: " + key);
        return false;
    }
    
    memcpy(fieldPtr, data, size);
    saveFields[key].isDirty = true;
    
    WISP_DEBUG_INFO("SAVE", "Blob field updated: " + key);
    return true;
}

WispSaveResult WispSaveSystem::reset() {
    // This would reset all fields to their default values
    // Implementation depends on whether we store default values
    for (auto& pair : saveFields) {
        pair.second.isDirty = true;
    }
    
    WISP_DEBUG_INFO("SAVE", "Save data reset to defaults");
    return SAVE_SUCCESS;
}

bool WispSaveSystem::hasSaveFile() const {
    String filePath = getSaveFilePath();
    
    if (useSDCard) {
        return SD.exists(filePath);
    } else {
        return SPIFFS.exists(filePath);
    }
}

bool WispSaveSystem::deleteSaveFile() {
    String filePath = getSaveFilePath();
    
    bool success;
    if (useSDCard) {
        success = SD.remove(filePath);
    } else {
        success = SPIFFS.remove(filePath);
    }
    
    if (success) {
        WISP_DEBUG_INFO("SAVE", "Save file deleted");
    } else {
        WISP_DEBUG_ERROR("SAVE", "Failed to delete save file");
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
            WISP_DEBUG_INFO("SAVE", "Auto-save triggered");
            save();
        }
        
        lastAutoSave = currentTime;
    }
}

void WispSaveSystem::createBackup() const {
    String savePath = getSaveFilePath();
    String backupPath = getBackupFilePath();
    
    // Simple file copy
    File source, dest;
    
    if (useSDCard) {
        source = SD.open(savePath, FILE_READ);
        dest = SD.open(backupPath, FILE_WRITE);
    } else {
        source = SPIFFS.open(savePath, FILE_READ);
        dest = SPIFFS.open(backupPath, FILE_WRITE);
    }
    
    if (source && dest) {
        while (source.available()) {
            dest.write(source.read());
        }
        WISP_DEBUG_INFO("SAVE", "Backup created");
    }
    
    if (source) source.close();
    if (dest) dest.close();
}

bool WispSaveSystem::restoreFromBackup() {
    String savePath = getSaveFilePath();
    String backupPath = getBackupFilePath();
    
    // Check if backup exists
    bool backupExists;
    if (useSDCard) {
        backupExists = SD.exists(backupPath);
    } else {
        backupExists = SPIFFS.exists(backupPath);
    }
    
    if (!backupExists) {
        WISP_DEBUG_ERROR("SAVE", "No backup file to restore from");
        return false;
    }
    
    // Copy backup to main save file
    File source, dest;
    
    if (useSDCard) {
        source = SD.open(backupPath, FILE_READ);
        dest = SD.open(savePath, FILE_WRITE);
    } else {
        source = SPIFFS.open(backupPath, FILE_READ);
        dest = SPIFFS.open(savePath, FILE_WRITE);
    }
    
    bool success = false;
    if (source && dest) {
        while (source.available()) {
            dest.write(source.read());
        }
        success = true;
        WISP_DEBUG_INFO("SAVE", "Backup restored");
    }
    
    if (source) source.close();
    if (dest) dest.close();
    
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
    Serial.println("=== Wisp Save System Status ===");
    Serial.print("App: ");
    Serial.print(currentApp.uuid);
    Serial.print(" v");
    Serial.println(currentApp.version);
    
    Serial.print("Storage: ");
    Serial.println(useSDCard ? "SD Card" : "SPIFFS");
    
    Serial.print("Auto-save: ");
    Serial.print(autoSave ? "Enabled" : "Disabled");
    if (autoSave) {
        Serial.print(" (");
        Serial.print(autoSaveInterval);
        Serial.print("ms)");
    }
    Serial.println();
    
    Serial.print("Save file exists: ");
    Serial.println(hasSaveFile() ? "Yes" : "No");
    
    Serial.print("Registered fields: ");
    Serial.println(saveFields.size());
    
    Serial.println("==============================");
}

void WispSaveSystem::printFieldStatus() const {
    Serial.println("=== Save Field Status ===");
    
    for (const auto& pair : saveFields) {
        const WispSaveField& field = pair.second;
        Serial.print(field.key);
        Serial.print(" [");
        
        switch (field.type) {
            case SAVE_TYPE_BOOL: Serial.print("BOOL"); break;
            case SAVE_TYPE_INT8: Serial.print("INT8"); break;
            case SAVE_TYPE_UINT8: Serial.print("UINT8"); break;
            case SAVE_TYPE_INT16: Serial.print("INT16"); break;
            case SAVE_TYPE_UINT16: Serial.print("UINT16"); break;
            case SAVE_TYPE_INT32: Serial.print("INT32"); break;
            case SAVE_TYPE_UINT32: Serial.print("UINT32"); break;
            case SAVE_TYPE_FLOAT: Serial.print("FLOAT"); break;
            case SAVE_TYPE_STRING: Serial.print("STRING"); break;
            case SAVE_TYPE_BLOB: Serial.print("BLOB"); break;
        }
        
        Serial.print("] ");
        Serial.print(field.isDirty ? "DIRTY" : "CLEAN");
        Serial.print(" (");
        Serial.print(field.size);
        Serial.println(" bytes)");
    }
    
    Serial.println("========================");
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
