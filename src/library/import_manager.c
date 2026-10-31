#include "import_manager.h"

#include <stdio.h>
#include <string.h>

static int copy_text(char *out, size_t cap, const char *value, int required) {
    if (!value) return !required;
    size_t n = strlen(value);
    if ((required && n == 0) || n >= cap) return 0;
    memcpy(out, value, n + 1);
    return 1;
}

static int persist(import_manager_t *manager, const char *now) {
    if (!manager || !manager->library || !manager->storage_path[0])
        return 0;
    if (now && !copy_text(manager->updated_at, sizeof(manager->updated_at), now, 1))
        return 0;
    if (manager->updated_at[0])
        snprintf(manager->library->updated_at, sizeof(manager->library->updated_at),
                 "%s", manager->updated_at);
    return personal_library_save(manager->storage_path, manager->library) ==
           LIBRARY_STORE_OK;
}

static int transition_allowed(library_item_status_t from,
                              library_item_status_t to) {
    if (from == to) return 1;
    switch (from) {
    case LIBRARY_ITEM_IMPORTED:
        return to == LIBRARY_ITEM_QUEUED || to == LIBRARY_ITEM_MISSING ||
               to == LIBRARY_ITEM_ERROR;
    case LIBRARY_ITEM_QUEUED:
        return to == LIBRARY_ITEM_DOWNLOADING || to == LIBRARY_ITEM_PAUSED ||
               to == LIBRARY_ITEM_ERROR;
    case LIBRARY_ITEM_DOWNLOADING:
        return to == LIBRARY_ITEM_PAUSED || to == LIBRARY_ITEM_VERIFYING ||
               to == LIBRARY_ITEM_ERROR;
    case LIBRARY_ITEM_PAUSED:
        return to == LIBRARY_ITEM_QUEUED || to == LIBRARY_ITEM_DOWNLOADING ||
               to == LIBRARY_ITEM_ERROR;
    case LIBRARY_ITEM_VERIFYING:
        return to == LIBRARY_ITEM_READY || to == LIBRARY_ITEM_ERROR ||
               to == LIBRARY_ITEM_PAUSED;
    case LIBRARY_ITEM_READY:
        return to == LIBRARY_ITEM_MISSING;
    case LIBRARY_ITEM_MISSING:
        return to == LIBRARY_ITEM_IMPORTED || to == LIBRARY_ITEM_ERROR;
    case LIBRARY_ITEM_ERROR:
        return to == LIBRARY_ITEM_QUEUED || to == LIBRARY_ITEM_IMPORTED;
    default:
        return 0;
    }
}

int import_manager_open(import_manager_t *manager, personal_library_t *library,
                        const char *storage_path, const char *now_iso8601) {
    if (!manager || !library || !storage_path || !storage_path[0] ||
        !now_iso8601 || !now_iso8601[0])
        return 0;
    memset(manager, 0, sizeof(*manager));
    if (!copy_text(manager->storage_path, sizeof(manager->storage_path),
                   storage_path, 1) ||
        !copy_text(manager->updated_at, sizeof(manager->updated_at),
                   now_iso8601, 1))
        return 0;

    manager->library = library;
    library_store_result_t loaded =
        personal_library_load(manager->storage_path, library);
    if (loaded == LIBRARY_STORE_IO_ERROR) {
        FILE *probe = fopen(manager->storage_path, "rb");
        if (probe) {
            fclose(probe);
            return 0;
        }
        personal_library_init(library, "retrotorrent-personal", now_iso8601);
        if (!persist(manager, now_iso8601))
            return 0;
        return 1;
    }
    if (loaded != LIBRARY_STORE_OK)
        return 0;

    /* A process may have stopped while a transfer or verifier was active.
       Resume is explicit: recovered active jobs become paused, never "done". */
    int recovered = 0;
    for (size_t i = 0; i < library->count; ++i) {
        library_item_t *item = &library->items[i];
        if (item->status == LIBRARY_ITEM_DOWNLOADING ||
            item->status == LIBRARY_ITEM_VERIFYING) {
            item->status = LIBRARY_ITEM_PAUSED;
            recovered = 1;
        }
    }
    return !recovered || persist(manager, now_iso8601);
}

import_manager_result_t import_manager_add(import_manager_t *manager,
                                            const char *id,
                                            const char *title,
                                            const char *platform,
                                            const library_import_source_t *source,
                                            const char *created_at) {
    if (!manager || !manager->library || !id || !title || !source ||
        !created_at || !created_at[0])
        return IMPORT_MANAGER_INVALID;
    if (personal_library_find(manager->library, id))
        return IMPORT_MANAGER_DUPLICATE;
    if (manager->library->count >= PERSONAL_LIBRARY_MAX_ITEMS)
        return IMPORT_MANAGER_FULL;

    library_item_t item;
    memset(&item, 0, sizeof(item));
    if (!copy_text(item.id, sizeof(item.id), id, 1) ||
        !copy_text(item.title, sizeof(item.title), title, 1) ||
        !copy_text(item.created_at, sizeof(item.created_at), created_at, 1))
        return IMPORT_MANAGER_INVALID;
    if (platform && platform[0] &&
        !copy_text(item.platform, sizeof(item.platform), platform, 1))
        return IMPORT_MANAGER_INVALID;
    if (library_import_source_set(&item.source, source->type, source->uri) !=
        LIBRARY_IMPORT_SOURCE_OK)
        return IMPORT_MANAGER_INVALID;

    item.status = LIBRARY_ITEM_IMPORTED;
    if (!personal_library_upsert(manager->library, &item))
        return IMPORT_MANAGER_FULL;
    if (!persist(manager, created_at)) {
        personal_library_remove(manager->library, id);
        return IMPORT_MANAGER_IO_ERROR;
    }
    return IMPORT_MANAGER_OK;
}

import_manager_result_t import_manager_set_status(import_manager_t *manager,
                                                   const char *id,
                                                   library_item_status_t status,
                                                   const char *now_iso8601) {
    if (!manager || !manager->library || !id ||
        !library_item_status_name(status))
        return IMPORT_MANAGER_INVALID;
    library_item_t *item = personal_library_find(manager->library, id);
    if (!item) return IMPORT_MANAGER_NOT_FOUND;
    if (!transition_allowed(item->status, status))
        return IMPORT_MANAGER_BAD_TRANSITION;

    library_item_status_t previous = item->status;
    item->status = status;
    if (!persist(manager, now_iso8601)) {
        item->status = previous;
        return IMPORT_MANAGER_IO_ERROR;
    }
    return IMPORT_MANAGER_OK;
}

import_manager_result_t import_manager_set_target(import_manager_t *manager,
                                                   const char *id,
                                                   const char *platform,
                                                   const char *module,
                                                   const char *target_uri,
                                                   const char *now_iso8601) {
    if (!manager || !manager->library || !id || !platform || !target_uri)
        return IMPORT_MANAGER_INVALID;
    library_item_t *item = personal_library_find(manager->library, id);
    if (!item) return IMPORT_MANAGER_NOT_FOUND;

    library_import_source_t checked;
    if (library_import_source_set(&checked, LIBRARY_IMPORT_LOCAL_FILE,
                                  target_uri) != LIBRARY_IMPORT_SOURCE_OK)
        return IMPORT_MANAGER_INVALID;

    library_item_t previous = *item;
    if (!copy_text(item->target_uri, sizeof(item->target_uri), target_uri, 1) ||
        !copy_text(item->target_platform, sizeof(item->target_platform), platform, 1) ||
        (module && module[0] &&
         !copy_text(item->target_module, sizeof(item->target_module), module, 1))) {
        *item = previous;
        return IMPORT_MANAGER_INVALID;
    }
    if (!persist(manager, now_iso8601)) {
        *item = previous;
        return IMPORT_MANAGER_IO_ERROR;
    }
    return IMPORT_MANAGER_OK;
}

import_manager_result_t import_manager_set_progress(import_manager_t *manager,
                                                     const char *id,
                                                     uint64_t completed,
                                                     uint64_t total,
                                                     int verified,
                                                     const char *now_iso8601) {
    if (!manager || !manager->library || !id || completed > total)
        return IMPORT_MANAGER_INVALID;
    library_item_t *item = personal_library_find(manager->library, id);
    if (!item) return IMPORT_MANAGER_NOT_FOUND;
    if (item->status != LIBRARY_ITEM_DOWNLOADING &&
        item->status != LIBRARY_ITEM_VERIFYING &&
        item->status != LIBRARY_ITEM_PAUSED)
        return IMPORT_MANAGER_BAD_TRANSITION;

    uint64_t old_completed = item->completed_bytes;
    uint64_t old_total = item->total_bytes;
    int old_verified = item->transfer_verified;
    item->completed_bytes = completed;
    item->total_bytes = total;
    item->transfer_verified = verified ? 1 : 0;
    if (!persist(manager, now_iso8601)) {
        item->completed_bytes = old_completed;
        item->total_bytes = old_total;
        item->transfer_verified = old_verified;
        return IMPORT_MANAGER_IO_ERROR;
    }
    return IMPORT_MANAGER_OK;
}

import_manager_result_t import_manager_remove(import_manager_t *manager,
                                               const char *id) {
    if (!manager || !manager->library || !id)
        return IMPORT_MANAGER_INVALID;
    personal_library_t backup = *manager->library;
    if (!personal_library_remove(manager->library, id))
        return IMPORT_MANAGER_NOT_FOUND;
    if (!persist(manager, NULL)) {
        *manager->library = backup;
        return IMPORT_MANAGER_IO_ERROR;
    }
    return IMPORT_MANAGER_OK;
}

const char *import_manager_result_string(import_manager_result_t result) {
    switch (result) {
    case IMPORT_MANAGER_OK: return "ok";
    case IMPORT_MANAGER_INVALID: return "invalid argument or source";
    case IMPORT_MANAGER_IO_ERROR: return "could not persist the personal library";
    case IMPORT_MANAGER_DUPLICATE: return "library item already exists";
    case IMPORT_MANAGER_FULL: return "personal library is full";
    case IMPORT_MANAGER_BAD_TRANSITION: return "invalid import state transition";
    case IMPORT_MANAGER_NOT_FOUND: return "library item not found";
    default: return "unknown import manager error";
    }
}
