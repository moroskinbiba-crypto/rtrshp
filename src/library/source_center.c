#include "source_center.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SOURCE_REGISTRY_MAX_BYTES (64u * 1024u)
#define SOURCE_JSON_MAX_DEPTH 32

typedef struct {
    const char *start;
    const char *end;
} json_span_t;

static const char *skip_ws(const char *p, const char *end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
        ++p;
    return p;
}

static const char *skip_string(const char *p, const char *end) {
    if (p >= end || *p != '"')
        return NULL;
    ++p;
    while (p < end) {
        unsigned char c = (unsigned char)*p++;
        if (c == '"')
            return p;
        if (c < 0x20)
            return NULL;
        if (c == '\\') {
            if (p >= end)
                return NULL;
            char escaped = *p++;
            if (escaped == 'u') {
                for (int i = 0; i < 4; ++i) {
                    if (p >= end || !isxdigit((unsigned char)*p++))
                        return NULL;
                }
            } else if (!(escaped == '"' || escaped == '\\' || escaped == '/' ||
                         escaped == 'b' || escaped == 'f' || escaped == 'n' ||
                         escaped == 'r' || escaped == 't')) {
                return NULL;
            }
        }
    }
    return NULL;
}

static const char *skip_value(const char *p, const char *end, unsigned depth) {
    if (depth > SOURCE_JSON_MAX_DEPTH)
        return NULL;
    p = skip_ws(p, end);
    if (p >= end)
        return NULL;

    if (*p == '"')
        return skip_string(p, end);

    if (*p == '{') {
        ++p;
        p = skip_ws(p, end);
        if (p < end && *p == '}')
            return p + 1;
        for (;;) {
            const char *key_end = skip_string(p, end);
            if (!key_end)
                return NULL;
            p = skip_ws(key_end, end);
            if (p >= end || *p++ != ':')
                return NULL;
            p = skip_value(p, end, depth + 1);
            if (!p)
                return NULL;
            p = skip_ws(p, end);
            if (p >= end)
                return NULL;
            if (*p == '}')
                return p + 1;
            if (*p++ != ',')
                return NULL;
            p = skip_ws(p, end);
        }
    }

    if (*p == '[') {
        ++p;
        p = skip_ws(p, end);
        if (p < end && *p == ']')
            return p + 1;
        for (;;) {
            p = skip_value(p, end, depth + 1);
            if (!p)
                return NULL;
            p = skip_ws(p, end);
            if (p >= end)
                return NULL;
            if (*p == ']')
                return p + 1;
            if (*p++ != ',')
                return NULL;
            p = skip_ws(p, end);
        }
    }

    const char *start = p;
    while (p < end && *p != ',' && *p != ']' && *p != '}' &&
           *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
        ++p;
    if (p == start)
        return NULL;
    return p;
}

static int json_string_copy(json_span_t span, char *out, size_t cap) {
    if (!span.start || !span.end || span.end <= span.start ||
        *span.start != '"' || span.end[-1] != '"' || cap == 0)
        return 0;

    const char *p = span.start + 1;
    const char *end = span.end - 1;
    size_t used = 0;
    while (p < end) {
        unsigned char c = (unsigned char)*p++;
        if (c == '\\') {
            /* Registry strings are plain UTF-8; reject escape sequences to
               keep path/URL interpretation exact and deterministic. */
            return 0;
        }
        if (c < 0x20 || used + 1 >= cap)
            return 0;
        out[used++] = (char)c;
    }
    out[used] = '\0';
    return 1;
}

static int json_object_get(json_span_t object, const char *key,
                           json_span_t *value) {
    if (!object.start || !object.end || *object.start != '{' ||
        object.end[-1] != '}')
        return 0;

    const char *p = object.start + 1;
    const char *end = object.end - 1;
    p = skip_ws(p, end);
    while (p < end) {
        const char *key_end = skip_string(p, end);
        if (!key_end)
            return 0;
        json_span_t key_span = {p, key_end};
        char decoded_key[96];
        if (!json_string_copy(key_span, decoded_key, sizeof(decoded_key)))
            return 0;

        p = skip_ws(key_end, end);
        if (p >= end || *p++ != ':')
            return 0;
        p = skip_ws(p, end);
        const char *value_end = skip_value(p, end, 0);
        if (!value_end)
            return 0;

        if (strcmp(decoded_key, key) == 0) {
            value->start = p;
            value->end = value_end;
            return 1;
        }
        p = skip_ws(value_end, end);
        if (p >= end)
            return 0;
        if (*p == '}')
            return 0;
        if (*p++ != ',')
            return 0;
        p = skip_ws(p, end);
    }
    return 0;
}

static int object_string(json_span_t object, const char *key,
                         char *out, size_t cap, int required) {
    json_span_t value = {0};
    if (!json_object_get(object, key, &value)) {
        if (!required && cap)
            out[0] = '\0';
        return !required;
    }
    return json_string_copy(value, out, cap);
}

static int object_boolean(json_span_t object, const char *key,
                          int *out, int required) {
    json_span_t value = {0};
    if (!json_object_get(object, key, &value)) {
        *out = 0;
        return !required;
    }
    if ((size_t)(value.end - value.start) == 4 &&
        memcmp(value.start, "true", 4) == 0) {
        *out = 1;
        return 1;
    }
    if ((size_t)(value.end - value.start) == 5 &&
        memcmp(value.start, "false", 5) == 0) {
        *out = 0;
        return 1;
    }
    return 0;
}

static int object_uint(json_span_t object, const char *key,
                       unsigned *out, int required) {
    json_span_t value = {0};
    if (!json_object_get(object, key, &value)) {
        *out = 0;
        return !required;
    }
    if (value.start == value.end)
        return 0;
    unsigned long parsed = 0;
    for (const char *p = value.start; p < value.end; ++p) {
        if (!isdigit((unsigned char)*p))
            return 0;
        parsed = parsed * 10 + (unsigned)(*p - '0');
        if (parsed > 1000)
            return 0;
    }
    *out = (unsigned)parsed;
    return 1;
}

static int parse_methods(json_span_t value, unsigned *methods) {
    if (!value.start || !value.end || *value.start != '[' ||
        value.end[-1] != ']')
        return 0;
    *methods = 0;
    const char *p = skip_ws(value.start + 1, value.end - 1);
    const char *end = value.end - 1;
    while (p < end) {
        const char *str_end = skip_string(p, end);
        if (!str_end)
            return 0;
        char method[64];
        if (!json_string_copy((json_span_t){p, str_end}, method, sizeof(method)))
            return 0;
        if (strcmp(method, "save_topic_reference") == 0)
            *methods |= SOURCE_IMPORT_TOPIC_REFERENCE;
        else if (strcmp(method, "import_torrent_file") == 0)
            *methods |= SOURCE_IMPORT_TORRENT_FILE;
        else if (strcmp(method, "import_magnet_uri") == 0)
            *methods |= SOURCE_IMPORT_MAGNET_URI;
        else
            return 0;

        p = skip_ws(str_end, end);
        if (p >= end)
            break;
        if (*p++ != ',')
            return 0;
        p = skip_ws(p, end);
    }
    return 1;
}

static int valid_https_url(const char *url) {
    return url && strncmp(url, "https://", 8) == 0 &&
           url[8] != '\0' && strchr(url + 8, ' ') == NULL;
}

static int parse_source(json_span_t object, source_center_source_t *source) {
    memset(source, 0, sizeof(*source));
    json_span_t methods = {0};
    if (!object_string(object, "id", source->id, sizeof(source->id), 1) ||
        !object_string(object, "title", source->title, sizeof(source->title), 1) ||
        !object_string(object, "kind", source->kind, sizeof(source->kind), 1) ||
        !object_uint(object, "priority", &source->priority, 1) ||
        !object_boolean(object, "enabled", &source->enabled, 1) ||
        !object_boolean(object, "automated_indexing",
                        &source->automated_indexing, 1) ||
        !object_string(object, "homepage", source->homepage,
                       sizeof(source->homepage), 1) ||
        !object_string(object, "topic_url_prefix", source->topic_url_prefix,
                       sizeof(source->topic_url_prefix), 0) ||
        !object_string(object, "notes", source->notes,
                       sizeof(source->notes), 0) ||
        !json_object_get(object, "import_methods", &methods) ||
        !parse_methods(methods, &source->import_methods))
        return 0;

    if (strcmp(source->kind, "external_tracker") != 0 &&
        strcmp(source->kind, "catalog_api") != 0 &&
        strcmp(source->kind, "local_folder") != 0)
        return 0;
    if (!valid_https_url(source->homepage) || source->id[0] == '\0' ||
        source->title[0] == '\0' || !source->enabled)
        return 0;
    if (source->topic_url_prefix[0] &&
        !valid_https_url(source->topic_url_prefix))
        return 0;
    if (source->automated_indexing)
        return 0;
    return 1;
}

static void sort_sources(source_center_registry_t *registry) {
    for (size_t i = 1; i < registry->count; ++i) {
        source_center_source_t value = registry->sources[i];
        size_t j = i;
        while (j > 0 && registry->sources[j - 1].priority < value.priority) {
            registry->sources[j] = registry->sources[j - 1];
            --j;
        }
        registry->sources[j] = value;
    }
}

source_center_result_t source_center_load(const char *path,
                                          source_center_registry_t *registry) {
    if (!path || !registry)
        return SOURCE_CENTER_INVALID;
    memset(registry, 0, sizeof(*registry));

    FILE *file = fopen(path, "rb");
    if (!file)
        return SOURCE_CENTER_IO_ERROR;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return SOURCE_CENTER_IO_ERROR;
    }
    long size = ftell(file);
    if (size < 0 || (unsigned long)size > SOURCE_REGISTRY_MAX_BYTES) {
        fclose(file);
        return SOURCE_CENTER_TOO_LARGE;
    }
    rewind(file);
    char *buffer = (char *)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(file);
        return SOURCE_CENTER_IO_ERROR;
    }
    size_t read = fread(buffer, 1, (size_t)size, file);
    int read_ok = !ferror(file) && read == (size_t)size;
    fclose(file);
    if (!read_ok) {
        free(buffer);
        return SOURCE_CENTER_IO_ERROR;
    }
    buffer[read] = '\0';

    const char *begin = buffer;
    const char *end = buffer + read;
    const char *root_end = skip_value(begin, end, 0);
    if (!root_end || skip_ws(root_end, end) != end || *begin != '{') {
        free(buffer);
        return SOURCE_CENTER_INVALID;
    }
    json_span_t root = {begin, root_end};
    json_span_t source_array = {0};
    char schema_version[8];
    json_span_t version = {0};
    if (!json_object_get(root, "schema_version", &version) ||
        (size_t)(version.end - version.start) != 1 ||
        version.start[0] != '1' ||
        !object_string(root, "primary_source", registry->primary_source,
                       sizeof(registry->primary_source), 1) ||
        !json_object_get(root, "sources", &source_array) ||
        *source_array.start != '[' || source_array.end[-1] != ']') {
        free(buffer);
        return SOURCE_CENTER_INVALID;
    }
    (void)schema_version;

    const char *p = skip_ws(source_array.start + 1, source_array.end - 1);
    const char *array_end = source_array.end - 1;
    while (p < array_end) {
        const char *item_end = skip_value(p, array_end, 0);
        if (!item_end || *p != '{') {
            free(buffer);
            memset(registry, 0, sizeof(*registry));
            return SOURCE_CENTER_INVALID;
        }
        if (registry->count >= SOURCE_CENTER_MAX_SOURCES) {
            free(buffer);
            memset(registry, 0, sizeof(*registry));
            return SOURCE_CENTER_FULL;
        }
        if (!parse_source((json_span_t){p, item_end},
                          &registry->sources[registry->count])) {
            free(buffer);
            memset(registry, 0, sizeof(*registry));
            return SOURCE_CENTER_INVALID;
        }
        for (size_t i = 0; i < registry->count; ++i) {
            if (strcmp(registry->sources[i].id,
                       registry->sources[registry->count].id) == 0) {
                free(buffer);
                memset(registry, 0, sizeof(*registry));
                return SOURCE_CENTER_INVALID;
            }
        }
        ++registry->count;
        p = skip_ws(item_end, array_end);
        if (p < array_end) {
            if (*p++ != ',') {
                free(buffer);
                memset(registry, 0, sizeof(*registry));
                return SOURCE_CENTER_INVALID;
            }
            p = skip_ws(p, array_end);
        }
    }
    free(buffer);

    if (registry->count == 0) {
        memset(registry, 0, sizeof(*registry));
        return SOURCE_CENTER_INVALID;
    }
    int primary_found = 0;
    for (size_t i = 0; i < registry->count; ++i) {
        if (strcmp(registry->sources[i].id, registry->primary_source) == 0) {
            registry->sources[i].is_primary = 1;
            primary_found = 1;
        }
    }
    if (!primary_found) {
        memset(registry, 0, sizeof(*registry));
        return SOURCE_CENTER_NOT_FOUND;
    }
    sort_sources(registry);
    return SOURCE_CENTER_OK;
}

size_t source_center_count(const source_center_registry_t *registry) {
    return registry ? registry->count : 0;
}

const source_center_source_t *source_center_at(
    const source_center_registry_t *registry, size_t index) {
    if (!registry || index >= registry->count)
        return NULL;
    return &registry->sources[index];
}

const source_center_source_t *source_center_find(
    const source_center_registry_t *registry, const char *source_id) {
    if (!registry || !source_id)
        return NULL;
    for (size_t i = 0; i < registry->count; ++i)
        if (strcmp(registry->sources[i].id, source_id) == 0)
            return &registry->sources[i];
    return NULL;
}

const source_center_source_t *source_center_primary(
    const source_center_registry_t *registry) {
    if (!registry)
        return NULL;
    for (size_t i = 0; i < registry->count; ++i)
        if (registry->sources[i].is_primary)
            return &registry->sources[i];
    return NULL;
}

static const source_center_source_t *check_source_method(
    const source_center_registry_t *registry, const char *source_id,
    unsigned method) {
    const source_center_source_t *source =
        source_center_find(registry, source_id);
    if (!source || !source->enabled || !(source->import_methods & method))
        return NULL;
    return source;
}

import_manager_result_t source_center_save_topic_reference(
    const source_center_registry_t *registry, import_manager_t *manager,
    const char *source_id, const char *item_id, const char *title,
    const char *platform, const char *topic_url, const char *created_at) {
    const source_center_source_t *source =
        check_source_method(registry, source_id, SOURCE_IMPORT_TOPIC_REFERENCE);
    if (!source)
        return IMPORT_MANAGER_INVALID;
    if (source->topic_url_prefix[0] &&
        strncmp(topic_url, source->topic_url_prefix,
                strlen(source->topic_url_prefix)) != 0)
        return IMPORT_MANAGER_INVALID;

    library_import_source_t import_source;
    if (library_import_source_set(&import_source, LIBRARY_IMPORT_TRACKER_TOPIC,
                                  topic_url) != LIBRARY_IMPORT_SOURCE_OK)
        return IMPORT_MANAGER_INVALID;
    return import_manager_add(manager, item_id, title, platform,
                              &import_source, created_at);
}

import_manager_result_t source_center_import_torrent_file(
    const source_center_registry_t *registry, import_manager_t *manager,
    const char *source_id, const char *item_id, const char *title,
    const char *platform, const char *torrent_path, const char *created_at) {
    if (!check_source_method(registry, source_id, SOURCE_IMPORT_TORRENT_FILE))
        return IMPORT_MANAGER_INVALID;
    library_import_source_t import_source;
    if (library_import_source_set(&import_source, LIBRARY_IMPORT_TORRENT_FILE,
                                  torrent_path) != LIBRARY_IMPORT_SOURCE_OK)
        return IMPORT_MANAGER_INVALID;
    return import_manager_add(manager, item_id, title, platform,
                              &import_source, created_at);
}

import_manager_result_t source_center_import_magnet_uri(
    const source_center_registry_t *registry, import_manager_t *manager,
    const char *source_id, const char *item_id, const char *title,
    const char *platform, const char *magnet_uri, const char *created_at) {
    if (!check_source_method(registry, source_id, SOURCE_IMPORT_MAGNET_URI))
        return IMPORT_MANAGER_INVALID;
    library_import_source_t import_source;
    if (library_import_source_set(&import_source, LIBRARY_IMPORT_MAGNET,
                                  magnet_uri) != LIBRARY_IMPORT_SOURCE_OK)
        return IMPORT_MANAGER_INVALID;
    return import_manager_add(manager, item_id, title, platform,
                              &import_source, created_at);
}

const char *source_center_result_string(source_center_result_t result) {
    switch (result) {
    case SOURCE_CENTER_OK: return "ok";
    case SOURCE_CENTER_INVALID: return "invalid source registry";
    case SOURCE_CENTER_IO_ERROR: return "could not read source registry";
    case SOURCE_CENTER_TOO_LARGE: return "source registry exceeds size limit";
    case SOURCE_CENTER_FULL: return "source registry exceeds source limit";
    case SOURCE_CENTER_NOT_FOUND: return "configured primary source was not found";
    case SOURCE_CENTER_METHOD_UNAVAILABLE: return "source does not support this import method";
    default: return "unknown source registry error";
    }
}
