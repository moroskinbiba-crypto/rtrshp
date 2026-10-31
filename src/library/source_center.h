#pragma once

#include <stddef.h>
#include "import_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SOURCE_CENTER_MAX_SOURCES 32
#define SOURCE_CENTER_ID_MAX 64
#define SOURCE_CENTER_TITLE_MAX 128
#define SOURCE_CENTER_URL_MAX 256
#define SOURCE_CENTER_NOTES_MAX 256

enum {
    SOURCE_IMPORT_TOPIC_REFERENCE = 1u << 0,
    SOURCE_IMPORT_TORRENT_FILE = 1u << 1,
    SOURCE_IMPORT_MAGNET_URI = 1u << 2
};

typedef struct {
    char id[SOURCE_CENTER_ID_MAX];
    char title[SOURCE_CENTER_TITLE_MAX];
    char kind[32];
    unsigned priority;
    int enabled;
    int automated_indexing;
    char homepage[SOURCE_CENTER_URL_MAX];
    char topic_url_prefix[SOURCE_CENTER_URL_MAX];
    char notes[SOURCE_CENTER_NOTES_MAX];
    unsigned import_methods;
    int is_primary;
} source_center_source_t;

typedef struct {
    char primary_source[SOURCE_CENTER_ID_MAX];
    size_t count;
    source_center_source_t sources[SOURCE_CENTER_MAX_SOURCES];
} source_center_registry_t;

typedef enum {
    SOURCE_CENTER_OK = 1,
    SOURCE_CENTER_INVALID = 0,
    SOURCE_CENTER_IO_ERROR = -1,
    SOURCE_CENTER_TOO_LARGE = -2,
    SOURCE_CENTER_FULL = -3,
    SOURCE_CENTER_NOT_FOUND = -4,
    SOURCE_CENTER_METHOD_UNAVAILABLE = -5
} source_center_result_t;

/* Load and validate the runtime JSON source registry (up to 64 KiB). */
source_center_result_t source_center_load(const char *path,
                                          source_center_registry_t *registry);

size_t source_center_count(const source_center_registry_t *registry);
const source_center_source_t *source_center_at(
    const source_center_registry_t *registry, size_t index);
const source_center_source_t *source_center_find(
    const source_center_registry_t *registry, const char *source_id);
const source_center_source_t *source_center_primary(
    const source_center_registry_t *registry);

/* Personal-library actions exposed to a future Borealis source/import screen. */
import_manager_result_t source_center_save_topic_reference(
    const source_center_registry_t *registry, import_manager_t *manager,
    const char *source_id, const char *item_id, const char *title,
    const char *platform, const char *topic_url, const char *created_at);

import_manager_result_t source_center_import_torrent_file(
    const source_center_registry_t *registry, import_manager_t *manager,
    const char *source_id, const char *item_id, const char *title,
    const char *platform, const char *torrent_path, const char *created_at);

import_manager_result_t source_center_import_magnet_uri(
    const source_center_registry_t *registry, import_manager_t *manager,
    const char *source_id, const char *item_id, const char *title,
    const char *platform, const char *magnet_uri, const char *created_at);

const char *source_center_result_string(source_center_result_t result);

#ifdef __cplusplus
}
#endif
