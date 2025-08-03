// file_system.h - ESP32 native file system wrapper
#pragma once
#include "../system/esp32_common.h"
#include <fstream>
#include <string>
#include <esp_vfs.h>
#include <esp_spiffs.h>

// File access modes compatible with Arduino
#define FILE_READ "r"
#define FILE_WRITE "w"
#define FILE_APPEND "a"

// ESP32 File wrapper class to replace Arduino File
class File {
private:
    std::fstream file;
    std::string path;
    bool isOpen;

public:
    File() : isOpen(false) {}
    File(const std::string& filepath, const char* mode) : path(filepath), isOpen(false) {
        open(filepath, mode);
    }
    
    bool open(const std::string& filepath, const char* mode) {
        std::ios_base::openmode iosMode = std::ios_base::in;
        if (strcmp(mode, "w") == 0) iosMode = std::ios_base::out;
        else if (strcmp(mode, "a") == 0) iosMode = std::ios_base::app;
        
        file.open(filepath, iosMode);
        isOpen = file.is_open();
        return isOpen;
    }
    
    void close() {
        if (isOpen) {
            file.close();
            isOpen = false;
        }
    }
    
    operator bool() const { return isOpen; }
    bool available() { return isOpen && file.good(); }
    
    std::string readString() {
        if (!isOpen) return "";
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return content;
    }
    
    size_t write(const std::string& data) {
        if (!isOpen) return 0;
        file << data;
        return data.length();
    }
    
    void flush() { if (isOpen) file.flush(); }
};

// ESP32 SPIFFS wrapper
class SPIFFSClass {
public:
    bool begin() {
        esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true
        };
        esp_err_t ret = esp_vfs_spiffs_register(&conf);
        return (ret == ESP_OK);
    }
    
    void end() {
        esp_vfs_spiffs_unregister(NULL);
    }
    
    File open(const std::string& path, const char* mode = FILE_READ) {
        std::string fullPath = "/spiffs" + path;
        return File(fullPath, mode);
    }
    
    bool exists(const std::string& path) {
        std::string fullPath = "/spiffs" + path;
        std::ifstream file(fullPath);
        return file.good();
    }
};

// ESP32 SD wrapper (basic implementation)
class SDClass {
public:
    bool begin() {
        // Basic SD initialization - would need proper SD card setup
        return false; // Disabled for now
    }
    
    File open(const std::string& path, const char* mode = FILE_READ) {
        std::string fullPath = "/sd" + path;
        return File(fullPath, mode);
    }
    
    bool exists(const std::string& path) {
        std::string fullPath = "/sd" + path;
        std::ifstream file(fullPath);
        return file.good();
    }
};

// Global instances
extern SPIFFSClass SPIFFS;
extern SDClass SD;
