#pragma once

#include "library_store.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    personal_library_t *library;
    char storage_path[PERSONAL_LIBRARY_PATH_MAX];
    char updated_at[PERSONAL_LIBRARY_TIMESTAMP_MAX];
} import_manager_t;

typedef enum {
    IMPORT_MANAGER_OK = 1,
    IMPORT_MANAGER_INVALID = 0,
    IMPORT_MANAGER_IO_ERROR = -1,
    IMPORT_MANAGER_DUPLICATE = -2,
    IMPORT_MANAGER_FULL = -3,
    IMPORT_MANAGER_BAD_TRANSITION = -4,
    IMPORT_MANAGER_NOT_FOUND = -5
} import_manager_result_t;

int import_manager_open(import_manager_t *manager, personal_library_t *library,
                        const char *storage_path, const char *now_iso8601);
import_manager_result_t import_manager_add(import_manager_t *manager,
                                            const char *id,
                                            const char *title,
                                            const char *platform,
                                            const library_import_source_t *source,
                                            const char *created_at);
import_manager_result_t import_manager_set_status(import_manager_t *manager,
                                                   const char *id,
                                                   library_item_status_t status,
                                                   const char *now_iso8601);
import_manager_result_t import_manager_set_target(import_manager_t *manager,
                                                   const char *id,
                                                   const char *platform,
                                                   const char *module,
                                                   const char *target_uri,
                                                   const char *now_iso8601);
import_manager_result_t import_manager_set_progress(import_manager_t *manager,
                                                     const char *id,
                                                     uint64_t completed,
                                                     uint64_t total,
                                                     int verified,
                                                     const char *now_iso8601);
import_manager_result_t import_manager_remove(import_manager_t *manager,
                                               const char *id);
const char *import_manager_result_string(import_manager_result_t result);

#ifdef __cplusplus
}
#endif
