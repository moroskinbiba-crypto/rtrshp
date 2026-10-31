#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIBRARY_IMPORT_URI_MAX 2048

typedef enum {
    LIBRARY_IMPORT_LOCAL_FILE = 0,
    LIBRARY_IMPORT_TORRENT_FILE = 1,
    LIBRARY_IMPORT_MAGNET = 2
} library_import_source_type_t;

typedef struct {
    library_import_source_type_t type;
    char uri[LIBRARY_IMPORT_URI_MAX];
} library_import_source_t;

typedef enum {
    LIBRARY_IMPORT_SOURCE_OK = 1,
    LIBRARY_IMPORT_SOURCE_INVALID_ARGUMENT = 0,
    LIBRARY_IMPORT_SOURCE_TOO_LONG = -1,
    LIBRARY_IMPORT_SOURCE_INVALID_PATH = -2,
    LIBRARY_IMPORT_SOURCE_INVALID_MAGNET = -3,
    LIBRARY_IMPORT_SOURCE_INVALID_TYPE = -4
} library_import_source_result_t;

/*
 * Validate and copy a user-selected import source.
 *
 * LOCAL_FILE and TORRENT_FILE accept absolute paths, including Switch device
 * paths such as sdmc:/... and usb:/..., and reject parent-directory segments.
 * MAGNET requires a magnet:? URI containing a valid BitTorrent v1 (40-char
 * hex or 32-char base32) xt=urn:btih: value.
 *
 * This function does not open files, parse torrent metainfo, access the
 * network, or add a job to the download queue.
 */
library_import_source_result_t library_import_source_set(
    library_import_source_t *out,
    library_import_source_type_t type,
    const char *uri);

const char *library_import_source_result_string(
    library_import_source_result_t result);

#ifdef __cplusplus
}
#endif
