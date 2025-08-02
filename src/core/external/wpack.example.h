#include "wpack.h"
#include <stdio.h>
#include <stdlib.h>

#define TEMP_BUFFER_SIZE 1024 * 4

void load_asset_example() {
    WPack pack;
    if (wpack_open(&pack, "/sdcard/assets.wpack") != 0) {
        printf("Failed to open .wpack\n");
        return;
    }

    const WPackEntry* ent = wpack_find(&pack, "bg_forest.sprite");
    if (ent) {
        uint8_t* buffer = malloc(ent->size);
        if (!buffer) {
            printf("Out of memory\n");
            return;
        }

        if (wpack_load(&pack, ent, buffer, ent->size) > 0) {
            // do something with buffer
        }

        free(buffer);
    } else {
        printf("Asset not found\n");
    }

    wpack_close(&pack);
}
