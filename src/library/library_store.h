#pragma once

#include <stddef.h>
#include <stdint.h>

#include "import_source.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PERSONAL_LIBRARY_MAX_ITEMS 256
#define PERSONAL_LIBRARY_ID_MAX 96
#define PERSONAL_LIBRARY_TITLE_MAX 256
#define PERSONAL_LIBRARY_PLATFORM_MAX 64
#define PERSONAL_LIBRARY_PATH_MAX 1024
#define PERSONAL_LIBRARY_TIMESTAMP_MAX 40
#define PERSONAL_LIBRARY_HASH_MAX 65

typedef enum {
    LIBRARY_ITEM_IMPORTED = 0,
    LIBRARY_ITEM_QUEUED,
    LIBRARY_ITEM_DOWNLOADING,
    LIBRARY_ITEM_PAUSED,
    LIBRARY_ITEM_VERIFYING,
    LIBRARY_ITEM_READY,
    LIBRARY_ITEM_MISSING,
    LIBRARY_ITEM_ERROR
} library_item_status_t;

typedef struct {
    char id[PERSONAL_LIBRARY_ID_MAX];
    char title[PERSONAL_LIBRARY_TITLE_MAX];
    char platform[PERSONAL_LIBRARY_PLATFORM_MAX];
    library_import_source_t source;
    library_item_status_t status;
    char created_at[PERSONAL_LIBRARY_TIMESTAMP_MAX];
    char target_uri[PERSONAL_LIBRARY_PATH_MAX];
    char target_platform[PERSONAL_LIBRARY_PLATFORM_MAX];
    char target_module[PERSONAL_LIBRARY_PLATFORM_MAX];
    char info_hash[PERSONAL_LIBRARY_HASH_MAX];
    uint64_t total_bytes;
    uint64_t completed_bytes;
    int transfer_verified;
} library_item_t;

typedef struct {
    char library_id[PERSONAL_LIBRARY_ID_MAX];
    char updated_at[PERSONAL_LIBRARY_TIMESTAMP_MAX];
    size_t count;
    library_item_t items[PERSONAL_LIBRARY_MAX_ITEMS];
} personal_library_t;

typedef enum {
    LIBRARY_STORE_OK = 1,
    LIBRARY_STORE_INVALID = 0,
    LIBRARY_STORE_IO_ERROR = -1,
    LIBRARY_STORE_TOO_LARGE = -2,
    LIBRARY_STORE_FULL = -3,
    LIBRARY_STORE_NOT_FOUND = -4
} library_store_result_t;

void personal_library_init(personal_library_t *library, const char *library_id,
                           const char *updated_at);
library_store_result_t personal_library_load(const char *path,
                                               personal_library_t *library);
library_store_result_t personal_library_save(const char *path,
                                               const personal_library_t *library);
library_item_t *personal_library_find(personal_library_t *library,
                                      const char *item_id);
const char *library_item_status_name(library_item_status_t status);
int library_item_status_parse(const char *name, library_item_status_t *status);
int personal_library_upsert(personal_library_t *library,
                            const library_item_t *item);
int personal_library_remove(personal_library_t *library, const char *item_id);

#ifdef __cplusplus
}
#endif
