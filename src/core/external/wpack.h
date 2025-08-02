// wpack.h
#pragma once
#include <stdint.h>
#include <stdio.h>

#define WPACK_MAGIC 0x314B5057  // "WPK1" little-endian
#define WPACK_MAX_ENTRIES 256   // or adjust based on your pack size
#define WPACK_NAME_LEN 32

typedef struct {
    char name[WPACK_NAME_LEN]; // null-terminated or padded
    uint32_t offset;
    uint32_t size;
    uint8_t type;
    uint8_t reserved[3];
} WPackEntry;

typedef struct {
    FILE* file;
    uint16_t entry_count;
    WPackEntry entries[WPACK_MAX_ENTRIES];
} WPack;
