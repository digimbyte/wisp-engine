// engine/save_system.cpp
#include "save_system.h"
#include "../../system/esp32_common.h"
#include "../../engine/core/debug.h"  // Use new debug system
#include <type_traits>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"

// Global save system instance
WispSaveSystem *g_SaveSystem = nullptr;

// Save result to string conversion
const char *getSaveResultString(WispSaveResult result) {
    switch (result)
    {
    case SAVE_SUCCESS:
        return "Success";
    case SAVE_ERROR_NO_STORAGE:
        return "No storage available";
    case SAVE_ERROR_WRITE_FAILED:
        return "Write failed";
    case SAVE_ERROR_READ_FAILED:
        return "Read failed";
    case SAVE_ERROR_INVALID_FILE:
        return "Invalid file format";
    case SAVE_ERROR_WRONG_APP:
        return "Save file belongs to different app";
    case SAVE_ERROR_VERSION_MISMATCH:
        return "Version mismatch";
    case SAVE_ERROR_CORRUPTED:
        return "Save file corrupted";
    case SAVE_ERROR_NOT_FOUND:
        return "Save file not found";
    case SAVE_ERROR_MEMORY_FULL:
        return "Memory full";
    default:
        return "Unknown error";
    }
}

// Initialize save system
bool WispSaveSystem::init(bool preferSDCard)
{
    useSDCard = preferSDCard;

    if (useSDCard)
    {
        // ESP-IDF SD card initialization would go here
        // For now, fall back to SPIFFS
        useSDCard = false;
    }

    if (!useSDCard)
    {
        // Initialize SPIFFS using ESP-IDF
        esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true};

        esp_err_t ret = esp_vfs_spiffs_register(&conf);
        if (ret != ESP_OK)
        {
            WISP_DEBUG_ERROR("SAVE", "Failed to initialize SPIFFS");
            return false;
        }
    }

    // Create save directory if it doesn't exist
    String storage = useSDCard ? "SD" : "SPIFFS";

    if (!useSDCard)
    {
        // For SPIFFS, ensure base path exists
        struct stat st;
        String fullSaveDir = String("/spiffs") + String(saveDirectory);
        if (stat(fullSaveDir.c_str(), &st) != 0)
        {
            mkdir(fullSaveDir.c_str(), 0755);
        }
    }

    char debugMsg[64];
    snprintf(debugMsg, sizeof(debugMsg), "Save system initialized using %s", storage.c_str());
    WISP_DEBUG_LOG("SAVE", debugMsg);
    return true;
}

void WispSaveSystem::setAppIdentity(const WispAppIdentity &identity)
{
    if (identity.uuid[0] == '\0')
    {
        WISP_DEBUG_ERROR("SAVE", "App UUID cannot be empty");
        return;
    }

    // Validate UUID format (basic check for reverse domain notation)
    if (strchr(identity.uuid, '.') == nullptr)
    {
        WISP_DEBUG_WARNING("SAVE", "App UUID should use reverse domain notation (e.g. com.developer.gamename)");
    }

    currentApp = identity;
    char debugMsg[128];
    snprintf(debugMsg, sizeof(debugMsg), "App identity set: %s v%s", identity.uuid, identity.version);
    WISP_DEBUG_LOG("SAVE", debugMsg);
}

void WispSaveSystem::setAutoSave(bool enabled, uint32_t intervalMs)
{
    autoSave = enabled;
    autoSaveInterval = intervalMs;
    lastAutoSave = get_millis();

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Auto-save %s%s",
             enabled ? "enabled" : "disabled",
             (enabled ? " (interval: " : ""));
    if (enabled)
    {
        char intervalStr[32];
        snprintf(intervalStr, sizeof(intervalStr), "%lums)", (unsigned long)intervalMs);
        strcat(buffer, intervalStr);
    }
    WISP_DEBUG_LOG("SAVE", buffer);
}

void WispSaveSystem::getSaveFilePath(char* buffer, size_t bufferSize) const
{
    // Create safe filename from UUID (replace dots and invalid chars)
    char safeUuid[64];
    strncpy(safeUuid, currentApp.uuid, sizeof(safeUuid) - 1);
    safeUuid[sizeof(safeUuid) - 1] = '\0';

    // Replace dots with underscores for safe filename
    for (char *p = safeUuid; *p; p++)
    {
        if (*p == '.')
            *p = '_';
    }
    // Replace slashes with underscores for safe filename
    for (char *p = safeUuid; *p; p++)
    {
        if (*p == '/' || *p == '\\')
            *p = '_';
    }

    if (useSDCard)
    {
        snprintf(buffer, bufferSize, "%s/%s.sav", saveDirectory, safeUuid);
    }
    else
    {
        snprintf(buffer, bufferSize, "/spiffs%s/%s.sav", saveDirectory, safeUuid);
    }
}

void WispSaveSystem::getBackupFilePath(char* buffer, size_t bufferSize) const
{
    char savePath[256];
    getSaveFilePath(savePath, sizeof(savePath));
    snprintf(buffer, bufferSize, "%s.bak", savePath);
}

uint32_t WispSaveSystem::calculateChecksum(const uint8_t *data, size_t size) const
{
    // Simple CRC32 implementation
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < size; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ 0xEDB88320;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

bool WispSaveSystem::validateSaveFile(const char* filePath, WispSaveHeader &header) const
{
    FILE *file = fopen(filePath, "rb");
    if (!file)
    {
        return false;
    }

    // Read header
    if (fread(&header, sizeof(WispSaveHeader), 1, file) != 1)
    {
        fclose(file);
        return false;
    }

    // Validate magic number
    if (header.magic != 0x57495350)
    {
        WISP_DEBUG_ERROR("SAVE", "Invalid magic number in save file");
        fclose(file);
        return false;
    }

    // Validate app identity
    if (strcmp(header.appUuid, currentApp.uuid) != 0)
    {
        char errorMsg[128];
        snprintf(errorMsg, sizeof(errorMsg), "Save file belongs to different app: %s", header.appUuid);
        WISP_DEBUG_ERROR("SAVE", errorMsg);
        fclose(file);
        return false;
    }

    // Validate fingerprint
    if (header.fingerprint != currentApp.generateFingerprint())
    {
        WISP_DEBUG_WARNING("SAVE", "Save file fingerprint mismatch - possible version issue");
    }

    // Get file size for validation
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, sizeof(WispSaveHeader), SEEK_SET);

    // Validate data section size
    if (header.dataSize > fileSize - sizeof(WispSaveHeader))
    {
        WISP_DEBUG_ERROR("SAVE", "Save file data size mismatch");
        fclose(file);
        return false;
    }

    // Read and validate checksum
    uint8_t *buffer = (uint8_t *)malloc(header.dataSize);
    if (!buffer)
    {
        WISP_DEBUG_ERROR("SAVE", "Cannot allocate memory for save data validation");
        fclose(file);
        return false;
    }

    if (fread(buffer, 1, header.dataSize, file) != header.dataSize)
    {
        WISP_DEBUG_ERROR("SAVE", "Cannot read save data for validation");
        free(buffer);
        fclose(file);
        return false;
    }

    uint32_t calculatedChecksum = calculateChecksum(buffer, header.dataSize);
    free(buffer);
    fclose(file);

    if (calculatedChecksum != header.checksum)
    {
        WISP_DEBUG_ERROR("SAVE", "Save file checksum mismatch - file may be corrupted");
        return false;
    }

    return true;
}

WispSaveResult WispSaveSystem::save()
{
    if (!isInitialized())
    {
        WISP_DEBUG_ERROR("SAVE", "Save system not initialized");
        return SAVE_ERROR_NO_STORAGE;
    }

    char filePath[256];
    getSaveFilePath(filePath, sizeof(filePath));

    // Create backup of existing save file
    if (hasSaveFile())
    {
        createBackup();
    }

    WispSaveResult result = writeSaveData(filePath);

    if (result == SAVE_SUCCESS)
    {
        markAllFieldsClean();
        lastAutoSave = get_millis();
        WISP_DEBUG_LOG("SAVE", "Save completed successfully");
    }
    else
    {
        char errorMsg[128];
        snprintf(errorMsg, sizeof(errorMsg), "Save failed: %s", getSaveResultString(result));
        WISP_DEBUG_ERROR("SAVE", errorMsg);

        // Try to restore from backup if save failed
        if (hasSaveFile())
        {
            restoreFromBackup();
        }
    }

    return result;
}

WispSaveResult WispSaveSystem::writeSaveData(const char* filePath) const
{
    // Calculate total data size
    size_t totalDataSize = 0;
    for (int i = 0; i < saveFieldCount; i++)
    {
        const WispSaveField &field = saveFields[i];
        totalDataSize += sizeof(uint8_t);    // Type
        totalDataSize += sizeof(uint16_t);   // Key length
        totalDataSize += strlen(field.key);  // Key string
        totalDataSize += sizeof(uint32_t);   // Data size

        if (field.type == SAVE_TYPE_STRING)
        {
            String *strPtr = static_cast<String *>(field.data);
            totalDataSize += strPtr ? strPtr->length() : 0;
        }
        else if (field.type == SAVE_TYPE_BLOB)
        {
            totalDataSize += field.size;
        }
        else
        {
            totalDataSize += field.size;
        }
    }

    // Allocate buffer for serialized data
    uint8_t *buffer = (uint8_t *)malloc(totalDataSize);
    if (!buffer)
    {
        WISP_DEBUG_ERROR("SAVE", "Cannot allocate memory for save data");
        return SAVE_ERROR_MEMORY_FULL;
    }

    // Serialize save fields
    uint8_t *ptr = buffer;
    for (int i = 0; i < saveFieldCount; i++)
    {
        const WispSaveField &field = saveFields[i];

        // Write type
        *ptr++ = (uint8_t)field.type;

        // Write key length and key
        uint16_t keyLen = strlen(field.key);
        memcpy(ptr, &keyLen, sizeof(uint16_t));
        ptr += sizeof(uint16_t);
        memcpy(ptr, field.key, keyLen);
        ptr += keyLen;

        // Write data size and data
        uint32_t dataSize;
        if (field.type == SAVE_TYPE_STRING)
        {
            char *strPtr = static_cast<char *>(field.data);
            dataSize = strPtr ? strlen(strPtr) : 0;
            memcpy(ptr, &dataSize, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            if (dataSize > 0)
            {
                memcpy(ptr, strPtr, dataSize);
                ptr += dataSize;
            }
        }
        else if (field.type == SAVE_TYPE_BLOB)
        {
            dataSize = field.size;
            memcpy(ptr, &dataSize, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            memcpy(ptr, field.data, dataSize);
            ptr += dataSize;
        }
        else
        {
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
    header.timestamp = get_millis();
    strncpy(header.appUuid, currentApp.uuid, sizeof(header.appUuid) - 1);
    strncpy(header.appVersion, currentApp.version, sizeof(header.appVersion) - 1);

    // Write to file
    FILE *file = fopen(filePath, "wb");
    if (!file)
    {
        free(buffer);
        char errorMsg[128];
        snprintf(errorMsg, sizeof(errorMsg), "Cannot open file for writing: %s", filePath);
        WISP_DEBUG_ERROR("SAVE", errorMsg);
        return SAVE_ERROR_WRITE_FAILED;
    }

    // Write header
    if (fwrite(&header, sizeof(WispSaveHeader), 1, file) != 1)
    {
        free(buffer);
        fclose(file);
        WISP_DEBUG_ERROR("SAVE", "Failed to write save header");
        return SAVE_ERROR_WRITE_FAILED;
    }

    // Write data
    if (fwrite(buffer, 1, totalDataSize, file) != totalDataSize)
    {
        free(buffer);
        fclose(file);
        WISP_DEBUG_ERROR("SAVE", "Failed to write save data");
        return SAVE_ERROR_WRITE_FAILED;
    }

    fclose(file);
    free(buffer);

    return SAVE_SUCCESS;
}

WispSaveResult WispSaveSystem::load()
{
    if (!isInitialized())
    {
        WISP_DEBUG_ERROR("SAVE", "Save system not initialized");
        return SAVE_ERROR_NO_STORAGE;
    }

    char filePath[256];
    getSaveFilePath(filePath, sizeof(filePath));
    return readSaveData(filePath);
}

WispSaveResult WispSaveSystem::readSaveData(const char* filePath)
{
    WispSaveHeader header;

    if (!validateSaveFile(filePath, header))
    {
        return SAVE_ERROR_INVALID_FILE;
    }

    FILE *file = fopen(filePath, "rb");
    if (!file)
    {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), "Cannot open save file for reading: %s", filePath);
        WISP_DEBUG_ERROR("SAVE", errorMsg);
        return SAVE_ERROR_READ_FAILED;
    }

    // Skip header
    fseek(file, sizeof(WispSaveHeader), SEEK_SET);

    // Read and deserialize data
    uint8_t *buffer = (uint8_t *)malloc(header.dataSize);
    if (!buffer)
    {
        fclose(file);
        WISP_DEBUG_ERROR("SAVE", "Cannot allocate memory for save data");
        return SAVE_ERROR_MEMORY_FULL;
    }

    if (fread(buffer, 1, header.dataSize, file) != header.dataSize)
    {
        free(buffer);
        fclose(file);
        WISP_DEBUG_ERROR("SAVE", "Failed to read save data");
        return SAVE_ERROR_READ_FAILED;
    }

    fclose(file);

    // Deserialize fields
    uint8_t *ptr = buffer;
    uint8_t *endPtr = buffer + header.dataSize;

    while (ptr < endPtr)
    {
        // Read type
        if (ptr >= endPtr)
            break;
        WispSaveDataType type = (WispSaveDataType)*ptr++;

        // Read key
        if (ptr + sizeof(uint16_t) > endPtr)
            break;
        uint16_t keyLen;
        memcpy(&keyLen, ptr, sizeof(uint16_t));
        ptr += sizeof(uint16_t);

        if (ptr + keyLen > endPtr)
            break;
        String key(reinterpret_cast<const char *>(ptr), static_cast<size_t>(keyLen));
        ptr += keyLen;

        // Read data size
        if (ptr + sizeof(uint32_t) > endPtr)
            break;
        uint32_t dataSize;
        memcpy(&dataSize, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        if (ptr + dataSize > endPtr)
            break;

        // Find field in our registry
        WispSaveField* foundField = nullptr;
        for (int i = 0; i < saveFieldCount; i++) {
            if (strcmp(saveFields[i].key, key.c_str()) == 0 && saveFields[i].type == type) {
                foundField = &saveFields[i];
                break;
            }
        }
        
        if (foundField)
        {
            if (foundField->type == SAVE_TYPE_STRING)
            {
                String *strPtr = static_cast<String *>(foundField->data);
                if (strPtr)
                {
                    *strPtr = String(reinterpret_cast<const char *>(ptr), static_cast<size_t>(dataSize));
                }
            }
            else if (foundField->type == SAVE_TYPE_BLOB)
            {
                if (dataSize <= foundField->size)
                {
                    memcpy(foundField->data, ptr, dataSize);
                }
            }
            else
            {
                if (dataSize == foundField->size)
                {
                    memcpy(foundField->data, ptr, dataSize);
                }
            }

            foundField->isDirty = false;
        }
        else
        {
            char debugMsg[128];
            snprintf(debugMsg, sizeof(debugMsg), "Unknown or type-mismatched field in save file: %s", key.c_str());
            WISP_DEBUG_WARNING("SAVE", debugMsg);
        }

        ptr += dataSize;
    }

    free(buffer);

    WISP_DEBUG_INFO("SAVE", "Save file loaded successfully");
    return SAVE_SUCCESS;
}

bool WispSaveSystem::registerStringField(const char* key, char* stringPtr, size_t maxLength)
{
    if (!stringPtr || hasField(key))
    {
        char debugMsg[128];
        snprintf(debugMsg, sizeof(debugMsg), "Invalid string pointer or field already exists: %s", key);
        WISP_DEBUG_ERROR("SAVE", debugMsg);
        return false;
    }

    if (saveFieldCount >= 64) {
        WISP_DEBUG_ERROR("SAVE", "Maximum number of save fields exceeded");
        return false;
    }

    saveFields[saveFieldCount] = WispSaveField(key, SAVE_TYPE_STRING, stringPtr, maxLength);
    saveFieldCount++;
    
    char debugMsg[128];
    snprintf(debugMsg, sizeof(debugMsg), "Registered string field: %s", key);
    WISP_DEBUG_INFO("SAVE", debugMsg);
    return true;
}

bool WispSaveSystem::registerBlobField(const char* key, void *dataPtr, size_t size)
{
    if (!dataPtr || size == 0 || hasField(key))
    {
        char debugMsg[128];
        snprintf(debugMsg, sizeof(debugMsg), "Invalid blob parameters or field already exists: %s", key);
        WISP_DEBUG_ERROR("SAVE", debugMsg);
        return false;
    }

    if (saveFieldCount >= 64) {
        WISP_DEBUG_ERROR("SAVE", "Maximum number of save fields exceeded");
        return false;
    }

    saveFields[saveFieldCount] = WispSaveField(key, SAVE_TYPE_BLOB, dataPtr, size);
    saveFieldCount++;

    char debugMsg[128];
    snprintf(debugMsg, sizeof(debugMsg), "Registered blob field: %s (%zu bytes)", key, size);
    WISP_DEBUG_INFO("SAVE", debugMsg);
    return true;
}

char* WispSaveSystem::getStringField(const char* key)
{
    for (int i = 0; i < saveFieldCount; i++) {
        if (strcmp(saveFields[i].key, key) == 0 && saveFields[i].type == SAVE_TYPE_STRING) {
            return static_cast<char*>(saveFields[i].data);
        }
    }
    
    char debugMsg[128];
    snprintf(debugMsg, sizeof(debugMsg), "String field not found: %s", key);
    WISP_DEBUG_WARNING("SAVE", debugMsg);
    return nullptr;
}

void *WispSaveSystem::getBlobField(const char* key, size_t *outSize)
{
    for (int i = 0; i < saveFieldCount; i++) {
        if (strcmp(saveFields[i].key, key) == 0 && saveFields[i].type == SAVE_TYPE_BLOB) {
            if (outSize) {
                *outSize = saveFields[i].size;
            }
            return saveFields[i].data;
        }
    }
    
    char debugMsg[128];
    snprintf(debugMsg, sizeof(debugMsg), "Blob field not found: %s", key);
    WISP_DEBUG_WARNING("SAVE", debugMsg);
    return nullptr;
}

bool WispSaveSystem::setStringField(const char* key, const char* value)
{
    char* fieldPtr = getStringField(key);
    if (!fieldPtr)
    {
        return false;
    }

    // Find the field to mark it dirty
    for (int i = 0; i < saveFieldCount; i++) {
        if (strcmp(saveFields[i].key, key) == 0) {
            // Copy the string safely
            strncpy(fieldPtr, value, saveFields[i].size - 1);
            fieldPtr[saveFields[i].size - 1] = '\0';
            saveFields[i].isDirty = true;
            break;
        }
    }

    char debugMsg[128];
    snprintf(debugMsg, sizeof(debugMsg), "String field updated: %s", key);
    WISP_DEBUG_INFO("SAVE", debugMsg);
    return true;
}

bool WispSaveSystem::setBlobField(const char* key, const void *data, size_t size)
{
    size_t maxSize;
    void *fieldPtr = getBlobField(key, &maxSize);
    if (!fieldPtr || size > maxSize)
    {
        char debugMsg[128];
        snprintf(debugMsg, sizeof(debugMsg), "Blob field not found or size too large: %s", key);
        WISP_DEBUG_ERROR("SAVE", debugMsg);
        return false;
    }

    memcpy(fieldPtr, data, size);
    
    // Find the field to mark it dirty
    for (int i = 0; i < saveFieldCount; i++) {
        if (strcmp(saveFields[i].key, key) == 0) {
            saveFields[i].isDirty = true;
            break;
        }
    }

    char debugMsg[128];
    snprintf(debugMsg, sizeof(debugMsg), "Blob field updated: %s", key);
    WISP_DEBUG_INFO("SAVE", debugMsg);
    return true;
}

WispSaveResult WispSaveSystem::reset()
{
    // This would reset all fields to their default values
    // Implementation depends on whether we store default values
    for (int i = 0; i < saveFieldCount; i++)
    {
        saveFields[i].isDirty = true;
    }

    WISP_DEBUG_INFO("SAVE", "Save data reset to defaults");
    return SAVE_SUCCESS;
}

bool WispSaveSystem::hasSaveFile() const
{
    char filePath[256];
    getSaveFilePath(filePath, sizeof(filePath));

    struct stat st;
    return (stat(filePath, &st) == 0);
}

bool WispSaveSystem::deleteSaveFile()
{
    char filePath[256];
    getSaveFilePath(filePath, sizeof(filePath));

    bool success = (remove(filePath) == 0);

    if (success)
    {
        WISP_DEBUG_INFO("SAVE", "Save file deleted");
    }
    else
    {
        WISP_DEBUG_ERROR("SAVE", "Failed to delete save file");
    }

    return success;
}

void WispSaveSystem::update()
{
    if (!autoSave || !isInitialized())
    {
        return;
    }

    uint32_t currentTime = get_millis();
    if (currentTime - lastAutoSave >= autoSaveInterval)
    {
        // Check if any fields are dirty
        bool needsSave = false;
        for (int i = 0; i < saveFieldCount; i++)
        {
            if (saveFields[i].isDirty)
            {
                needsSave = true;
                break;
            }
        }

        if (needsSave)
        {
            WISP_DEBUG_INFO("SAVE", "Auto-save triggered");
            save();
        }

        lastAutoSave = currentTime;
    }
}

void WispSaveSystem::createBackup() const
{
    char savePath[256];
    char backupPath[256];
    getSaveFilePath(savePath, sizeof(savePath));
    getBackupFilePath(backupPath, sizeof(backupPath));

    // Simple file copy using standard FILE operations
    FILE *source = fopen(savePath, "rb");
    FILE *dest = fopen(backupPath, "wb");

    if (source && dest)
    {
        char buffer[1024];
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), source)) > 0)
        {
            fwrite(buffer, 1, bytesRead, dest);
        }
        WISP_DEBUG_INFO("SAVE", "Backup created");
    }

    if (source)
        fclose(source);
    if (dest)
        fclose(dest);
}

bool WispSaveSystem::restoreFromBackup()
{
    char savePath[256];
    char backupPath[256];
    getSaveFilePath(savePath, sizeof(savePath));
    getBackupFilePath(backupPath, sizeof(backupPath));

    // Check if backup exists
    struct stat st;
    bool backupExists = (stat(backupPath, &st) == 0);

    if (!backupExists)
    {
        WISP_DEBUG_ERROR("SAVE", "No backup file to restore from");
        return false;
    }

    // Copy backup to main save file
    FILE *source = fopen(backupPath, "rb");
    FILE *dest = fopen(savePath, "wb");

    bool success = false;
    if (source && dest)
    {
        char buffer[1024];
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), source)) > 0)
        {
            fwrite(buffer, 1, bytesRead, dest);
        }
        success = true;
        WISP_DEBUG_INFO("SAVE", "Backup restored");
    }

    if (source)
        fclose(source);
    if (dest)
        fclose(dest);

    return success;
}

// Additional utility methods
bool WispSaveSystem::hasField(const char* key) const
{
    for (int i = 0; i < saveFieldCount; i++) {
        if (strcmp(saveFields[i].key, key) == 0) {
            return true;
        }
    }
    return false;
}

WispSaveDataType WispSaveSystem::getFieldType(const char* key) const
{
    for (int i = 0; i < saveFieldCount; i++) {
        if (strcmp(saveFields[i].key, key) == 0) {
            return saveFields[i].type;
        }
    }
    return SAVE_TYPE_BOOL;
}

bool WispSaveSystem::isFieldDirty(const char* key) const
{
    for (int i = 0; i < saveFieldCount; i++) {
        if (strcmp(saveFields[i].key, key) == 0) {
            return saveFields[i].isDirty;
        }
    }
    return false;
}

void WispSaveSystem::markFieldClean(const char* key)
{
    for (int i = 0; i < saveFieldCount; i++) {
        if (strcmp(saveFields[i].key, key) == 0) {
            saveFields[i].isDirty = false;
            break;
        }
    }
}

void WispSaveSystem::markAllFieldsClean()
{
    for (int i = 0; i < saveFieldCount; i++)
    {
        saveFields[i].isDirty = false;
    }
}

void WispSaveSystem::printSaveStatus() const
{
    printf("=== Wisp Save System Status ===\n");
    printf("App: %s v%s\n", currentApp.uuid, currentApp.version);

    printf("Storage: %s\n", useSDCard ? "SD Card" : "SPIFFS");

    printf("Auto-save: %s", autoSave ? "Enabled" : "Disabled");
    if (autoSave)
    {
        printf(" (%lu ms)", autoSaveInterval);
    }
    printf("\n");

    printf("Save file exists: %s\n", hasSaveFile() ? "Yes" : "No");

    printf("Registered fields: %d\n", saveFieldCount);

    printf("==============================\n");
}

void WispSaveSystem::printFieldStatus() const
{
    printf("=== Save Field Status ===\n");

    for (int i = 0; i < saveFieldCount; i++)
    {
        const WispSaveField &field = saveFields[i];
        printf("%s [", field.key);

        switch (field.type)
        {
        case SAVE_TYPE_BOOL:
            printf("BOOL");
            break;
        case SAVE_TYPE_INT8:
            printf("INT8");
            break;
        case SAVE_TYPE_UINT8:
            printf("UINT8");
            break;
        case SAVE_TYPE_INT16:
            printf("INT16");
            break;
        case SAVE_TYPE_UINT16:
            printf("UINT16");
            break;
        case SAVE_TYPE_INT32:
            printf("INT32");
            break;
        case SAVE_TYPE_UINT32:
            printf("UINT32");
            break;
        case SAVE_TYPE_FLOAT:
            printf("FLOAT");
            break;
        case SAVE_TYPE_STRING:
            printf("STRING");
            break;
        case SAVE_TYPE_BLOB:
            printf("BLOB");
            break;
        }

        printf("] %s (%zu bytes)\n",
               field.isDirty ? "DIRTY" : "CLEAN",
               field.size);
    }

    printf("========================\n");
}

size_t WispSaveSystem::getMemoryUsage() const
{
    size_t total = sizeof(WispSaveSystem);

    for (int i = 0; i < saveFieldCount; i++)
    {
        const WispSaveField &field = saveFields[i];
        total += strlen(field.key);     // Key string
        total += sizeof(WispSaveField); // Field structure
        // Note: We don't count the actual data since it's owned by the app
    }

    return total;
}
