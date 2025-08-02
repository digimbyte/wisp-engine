// wpack.c
#include "wpack.h"
#include <string.h>

int wpack_open(WPack* wp, const char* filepath) {
    uint32_t magic = 0;
    uint16_t count = 0;

    wp->file = fopen(filepath, "rb");
    if (!wp->file) return -1;

    fread(&magic, 4, 1, wp->file);
    if (magic != WPACK_MAGIC) return -2;

    fread(&count, 2, 1, wp->file);  // entry count
    fseek(wp->file, 2, SEEK_CUR);   // skip reserved

    wp->entry_count = count;
    if (count > WPACK_MAX_ENTRIES) return -3;

    for (int i = 0; i < count; ++i) {
        fread(&wp->entries[i], sizeof(WPackEntry), 1, wp->file);
    }

    return 0;
}

const WPackEntry* wpack_find(WPack* wp, const char* name) {
    for (int i = 0; i < wp->entry_count; ++i) {
        if (strncmp(name, wp->entries[i].name, WPACK_NAME_LEN) == 0) {
            return &wp->entries[i];
        }
    }
    return NULL;
}

int wpack_load(WPack* wp, const WPackEntry* entry, void* buffer, uint32_t max_len) {
    if (!entry || entry->size > max_len) return -1;

    fseek(wp->file, entry->offset, SEEK_SET);
    return fread(buffer, 1, entry->size, wp->file);
}

void wpack_close(WPack* wp) {
    if (wp->file) fclose(wp->file);
    wp->file = NULL;
}
