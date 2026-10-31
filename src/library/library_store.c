#include "library_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define library_mkdir(path) _mkdir(path)
#else
#define library_mkdir(path) mkdir((path), 0755)
#endif

#define LIBRARY_JSON_MAX (2u * 1024u * 1024u)
#define JSON_TOKEN_MAX 32768
#define LIBRARY_TMP_SUFFIX ".tmp"

typedef enum { JT_OBJECT, JT_ARRAY, JT_STRING, JT_PRIMITIVE } json_type_t;
typedef struct {
    size_t start, end, next;
    json_type_t type;
} json_token_t;

typedef struct {
    const char *src;
    size_t len, pos, count;
    json_token_t tokens[JSON_TOKEN_MAX];
    int failed;
} json_parser_t;


static int library_mkdir_checked(const char *path) {
    struct stat st;
    if (library_mkdir(path) == 0)
        return 1;
    if (errno != EEXIST)
        return 0;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int library_ensure_parent(const char *file_path) {
    char tmp[PERSONAL_LIBRARY_PATH_MAX + 8];
    int n = snprintf(tmp, sizeof(tmp), "%s", file_path);
    if (n < 0 || (size_t)n >= sizeof(tmp))
        return 0;

    char *last = strrchr(tmp, '/');
    if (!last)
        return 1;
    if (last == tmp)
        return 1;
    *last = '\\0';

    char *start = tmp + 1;
    char *colon = strchr(tmp, ':');
    if (colon && colon[1] == '/')
        start = colon + 2;

    for (char *p = start; *p; ++p) {
        if (*p != '/')
            continue;
        *p = '\\0';
        if (!library_mkdir_checked(tmp)) {
            *p = '/';
            return 0;
        }
        *p = '/';
    }
    return library_mkdir_checked(tmp);
}

static void skip_ws(json_parser_t *p) {
    while (p->pos < p->len &&
           (p->src[p->pos] == ' ' || p->src[p->pos] == '\t' ||
            p->src[p->pos] == '\r' || p->src[p->pos] == '\n'))
        ++p->pos;
}

static size_t token_new(json_parser_t *p, json_type_t type, size_t start) {
    if (p->count >= JSON_TOKEN_MAX) {
        p->failed = 1;
        return 0;
    }
    size_t n = p->count++;
    p->tokens[n].type = type;
    p->tokens[n].start = start;
    p->tokens[n].end = start;
    p->tokens[n].next = n + 1;
    return n;
}

static int parse_string_token(json_parser_t *p, size_t *out) {
    if (p->pos >= p->len || p->src[p->pos] != '"')
        return 0;
    ++p->pos;
    size_t t = token_new(p, JT_STRING, p->pos);
    while (p->pos < p->len) {
        unsigned char c = (unsigned char)p->src[p->pos++];
        if (c == '"') {
            p->tokens[t].end = p->pos - 1;
            p->tokens[t].next = p->count;
            *out = t;
            return 1;
        }
        if (c < 0x20)
            return 0;
        if (c == '\\') {
            if (p->pos >= p->len)
                return 0;
            char e = p->src[p->pos++];
            if (e == 'u') {
                for (int i = 0; i < 4; ++i) {
                    if (p->pos >= p->len ||
                        !isxdigit((unsigned char)p->src[p->pos++]))
                        return 0;
                }
            } else if (!(e == '"' || e == '\\' || e == '/' || e == 'b' ||
                         e == 'f' || e == 'n' || e == 'r' || e == 't')) {
                return 0;
            }
        }
    }
    return 0;
}

static int parse_value(json_parser_t *p, size_t *out, unsigned depth);

static int parse_container(json_parser_t *p, size_t *out, unsigned depth,
                           json_type_t type) {
    char open = type == JT_OBJECT ? '{' : '[';
    char close = type == JT_OBJECT ? '}' : ']';
    if (p->src[p->pos] != open)
        return 0;
    ++p->pos;
    size_t t = token_new(p, type, p->pos - 1);
    skip_ws(p);
    if (p->pos < p->len && p->src[p->pos] == close) {
        ++p->pos;
        p->tokens[t].end = p->pos;
        p->tokens[t].next = p->count;
        *out = t;
        return 1;
    }
    for (;;) {
        skip_ws(p);
        size_t child;
        if (type == JT_OBJECT) {
            if (!parse_string_token(p, &child))
                return 0;
            skip_ws(p);
            if (p->pos >= p->len || p->src[p->pos++] != ':')
                return 0;
            skip_ws(p);
            if (!parse_value(p, &child, depth + 1))
                return 0;
        } else if (!parse_value(p, &child, depth + 1)) {
            return 0;
        }
        skip_ws(p);
        if (p->pos >= p->len)
            return 0;
        char c = p->src[p->pos++];
        if (c == close)
            break;
        if (c != ',')
            return 0;
    }
    p->tokens[t].end = p->pos;
    p->tokens[t].next = p->count;
    *out = t;
    return 1;
}

static int parse_value(json_parser_t *p, size_t *out, unsigned depth) {
    if (depth > 32 || p->pos >= p->len)
        return 0;
    skip_ws(p);
    if (p->pos >= p->len)
        return 0;
    char c = p->src[p->pos];
    if (c == '"')
        return parse_string_token(p, out);
    if (c == '{')
        return parse_container(p, out, depth, JT_OBJECT);
    if (c == '[')
        return parse_container(p, out, depth, JT_ARRAY);

    size_t start = p->pos;
    while (p->pos < p->len) {
        c = p->src[p->pos];
        if (c == ',' || c == ']' || c == '}' || c == ' ' || c == '\t' ||
            c == '\r' || c == '\n')
            break;
        ++p->pos;
    }
    if (p->pos == start)
        return 0;
    size_t t = token_new(p, JT_PRIMITIVE, start);
    p->tokens[t].end = p->pos;
    p->tokens[t].next = p->count;
    *out = t;
    return 1;
}

static int json_parse(json_parser_t *p, const char *src, size_t len,
                      size_t *root) {
    memset(p, 0, sizeof(*p));
    p->src = src;
    p->len = len;
    skip_ws(p);
    if (!parse_value(p, root, 0) || p->failed)
        return 0;
    skip_ws(p);
    return p->pos == p->len;
}

static int token_string_eq(const json_parser_t *p, size_t t,
                           const char *value) {
    size_t n = strlen(value);
    return p->tokens[t].type == JT_STRING &&
           p->tokens[t].end - p->tokens[t].start == n &&
           memcmp(p->src + p->tokens[t].start, value, n) == 0;
}

static int object_get(const json_parser_t *p, size_t object,
                      const char *key, size_t *value) {
    if (p->tokens[object].type != JT_OBJECT)
        return 0;
    size_t i = object + 1;
    while (i < p->tokens[object].next) {
        size_t k = i;
        size_t v = k + 1;
        if (v >= p->tokens[object].next)
            return 0;
        if (token_string_eq(p, k, key)) {
            *value = v;
            return 1;
        }
        i = p->tokens[v].next;
    }
    return 0;
}

static int append_utf8(char *out, size_t cap, size_t *used, unsigned cp) {
    unsigned char bytes[3];
    size_t n;
    if (cp <= 0x7f) { bytes[0] = (unsigned char)cp; n = 1; }
    else if (cp <= 0x7ff) {
        bytes[0] = (unsigned char)(0xc0 | (cp >> 6));
        bytes[1] = (unsigned char)(0x80 | (cp & 0x3f)); n = 2;
    } else if (cp <= 0xffff && !(cp >= 0xd800 && cp <= 0xdfff)) {
        bytes[0] = (unsigned char)(0xe0 | (cp >> 12));
        bytes[1] = (unsigned char)(0x80 | ((cp >> 6) & 0x3f));
        bytes[2] = (unsigned char)(0x80 | (cp & 0x3f)); n = 3;
    } else return 0;
    if (*used + n >= cap)
        return 0;
    memcpy(out + *used, bytes, n);
    *used += n;
    return 1;
}

static int hex4(const char *s, unsigned *value) {
    unsigned v = 0;
    for (int i = 0; i < 4; ++i) {
        unsigned char c = (unsigned char)s[i];
        v <<= 4;
        if (c >= '0' && c <= '9') v |= c - '0';
        else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
        else return 0;
    }
    *value = v;
    return 1;
}

static int token_copy_string(const json_parser_t *p, size_t t,
                             char *out, size_t cap) {
    if (p->tokens[t].type != JT_STRING || cap == 0)
        return 0;
    size_t used = 0;
    for (size_t i = p->tokens[t].start; i < p->tokens[t].end; ++i) {
        unsigned char c = (unsigned char)p->src[i];
        if (c != '\\') {
            if (used + 1 >= cap) return 0;
            out[used++] = (char)c;
            continue;
        }
        if (++i >= p->tokens[t].end) return 0;
        char e = p->src[i];
        switch (e) {
        case '"': case '\\': case '/':
            if (used + 1 >= cap) return 0;
            out[used++] = e; break;
        case 'b': case 'f': case 'n': case 'r': case 't':
            if (used + 1 >= cap) return 0;
            out[used++] = e == 'b' ? '\b' : e == 'f' ? '\f' :
                          e == 'n' ? '\n' : e == 'r' ? '\r' : '\t';
            break;
        case 'u': {
            unsigned cp;
            if (i + 4 >= p->tokens[t].end ||
                !hex4(p->src + i + 1, &cp) ||
                !append_utf8(out, cap, &used, cp))
                return 0;
            i += 4;
            break;
        }
        default: return 0;
        }
    }
    out[used] = '\0';
    return 1;
}

static int object_string(const json_parser_t *p, size_t object,
                         const char *key, char *out, size_t cap,
                         int required) {
    size_t t;
    if (!object_get(p, object, key, &t))
        return !required;
    return token_copy_string(p, t, out, cap);
}

static int object_u64(const json_parser_t *p, size_t object,
                      const char *key, uint64_t *out) {
    size_t t;
    if (!object_get(p, object, key, &t))
        return 1;
    if (p->tokens[t].type != JT_PRIMITIVE)
        return 0;
    char num[32];
    size_t n = p->tokens[t].end - p->tokens[t].start;
    if (!n || n >= sizeof(num)) return 0;
    memcpy(num, p->src + p->tokens[t].start, n);
    num[n] = '\0';
    for (size_t i = 0; i < n; ++i)
        if (!isdigit((unsigned char)num[i])) return 0;
    *out = (uint64_t)strtoull(num, NULL, 10);
    return 1;
}

static int source_type_parse(const char *s, library_import_source_type_t *t) {
    if (strcmp(s, "local_file") == 0) *t = LIBRARY_IMPORT_LOCAL_FILE;
    else if (strcmp(s, "torrent_file") == 0) *t = LIBRARY_IMPORT_TORRENT_FILE;
    else if (strcmp(s, "magnet") == 0) *t = LIBRARY_IMPORT_MAGNET;
    else return 0;
    return 1;
}

static const char *source_type_name(library_import_source_type_t t) {
    switch (t) {
    case LIBRARY_IMPORT_LOCAL_FILE: return "local_file";
    case LIBRARY_IMPORT_TORRENT_FILE: return "torrent_file";
    case LIBRARY_IMPORT_MAGNET: return "magnet";
    default: return NULL;
    }
}

const char *library_item_status_name(library_item_status_t status) {
    static const char *names[] = {
        "imported", "queued", "downloading", "paused",
        "verifying", "ready", "missing", "error"
    };
    return status >= LIBRARY_ITEM_IMPORTED && status <= LIBRARY_ITEM_ERROR
        ? names[status] : NULL;
}

int library_item_status_parse(const char *name, library_item_status_t *status) {
    for (int i = LIBRARY_ITEM_IMPORTED; i <= LIBRARY_ITEM_ERROR; ++i) {
        if (strcmp(name, library_item_status_name((library_item_status_t)i)) == 0) {
            *status = (library_item_status_t)i;
            return 1;
        }
    }
    return 0;
}

void personal_library_init(personal_library_t *library, const char *library_id,
                           const char *updated_at) {
    if (!library) return;
    memset(library, 0, sizeof(*library));
    if (library_id) snprintf(library->library_id, sizeof(library->library_id), "%s", library_id);
    if (updated_at) snprintf(library->updated_at, sizeof(library->updated_at), "%s", updated_at);
}

library_item_t *personal_library_find(personal_library_t *library,
                                      const char *item_id) {
    if (!library || !item_id) return NULL;
    for (size_t i = 0; i < library->count; ++i)
        if (strcmp(library->items[i].id, item_id) == 0)
            return &library->items[i];
    return NULL;
}

int personal_library_upsert(personal_library_t *library,
                            const library_item_t *item) {
    if (!library || !item || !item->id[0] || !item->title[0])
        return 0;
    library_item_t *found = personal_library_find(library, item->id);
    if (found) {
        *found = *item;
        return 1;
    }
    if (library->count >= PERSONAL_LIBRARY_MAX_ITEMS)
        return 0;
    library->items[library->count++] = *item;
    return 1;
}

int personal_library_remove(personal_library_t *library, const char *item_id) {
    if (!library || !item_id) return 0;
    for (size_t i = 0; i < library->count; ++i) {
        if (strcmp(library->items[i].id, item_id) == 0) {
            if (i + 1 < library->count)
                memmove(&library->items[i], &library->items[i + 1],
                        (library->count - i - 1) * sizeof(library_item_t));
            --library->count;
            memset(&library->items[library->count], 0, sizeof(library_item_t));
            return 1;
        }
    }
    return 0;
}

static int parse_item(const json_parser_t *p, size_t object,
                      library_item_t *item) {
    memset(item, 0, sizeof(*item));
    char status[32], source_type[32];
    size_t source, target, transfer;

    if (p->tokens[object].type != JT_OBJECT ||
        !object_string(p, object, "id", item->id, sizeof(item->id), 1) ||
        !object_string(p, object, "title", item->title, sizeof(item->title), 1) ||
        !object_string(p, object, "platform", item->platform, sizeof(item->platform), 0) ||
        !object_string(p, object, "created_at", item->created_at, sizeof(item->created_at), 1) ||
        !object_string(p, object, "status", status, sizeof(status), 1) ||
        !library_item_status_parse(status, &item->status) ||
        !object_get(p, object, "source", &source) ||
        !object_string(p, source, "type", source_type, sizeof(source_type), 1) ||
        !source_type_parse(source_type, &item->source.type) ||
        !object_string(p, source, "uri", item->source.uri, sizeof(item->source.uri), 1))
        return 0;

    library_import_source_t checked_source;
    if (library_import_source_set(&checked_source, item->source.type,
                                  item->source.uri) != LIBRARY_IMPORT_SOURCE_OK)
        return 0;
    item->source = checked_source;
    if (object_get(p, object, "target", &target)) {
        if (!object_string(p, target, "uri", item->target_uri,
                           sizeof(item->target_uri), 0) ||
            !object_string(p, target, "platform", item->target_platform,
                           sizeof(item->target_platform), 0) ||
            !object_string(p, target, "module", item->target_module,
                           sizeof(item->target_module), 0))
            return 0;
    }
    if (object_get(p, object, "transfer", &transfer)) {
        char hash[PERSONAL_LIBRARY_HASH_MAX];
        hash[0] = '\0';
        if (!object_string(p, transfer, "info_hash", hash, sizeof(hash), 0) ||
            !object_u64(p, transfer, "total_bytes", &item->total_bytes) ||
            !object_u64(p, transfer, "completed_bytes", &item->completed_bytes))
            return 0;
        snprintf(item->info_hash, sizeof(item->info_hash), "%s", hash);
        size_t verified;
        if (object_get(p, transfer, "verified", &verified)) {
            item->transfer_verified = p->tokens[verified].type == JT_PRIMITIVE &&
                (p->tokens[verified].end - p->tokens[verified].start == 4) &&
                memcmp(p->src + p->tokens[verified].start, "true", 4) == 0;
        }
    }
    if (item->completed_bytes > item->total_bytes && item->total_bytes != 0)
        return 0;
    return 1;
}

library_store_result_t personal_library_load(const char *path,
                                               personal_library_t *library) {
    if (!path || !library) return LIBRARY_STORE_INVALID;
    FILE *f = fopen(path, "rb");
    if (!f) return LIBRARY_STORE_IO_ERROR;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return LIBRARY_STORE_IO_ERROR; }
    long file_size = ftell(f);
    if (file_size < 0 || (unsigned long)file_size > LIBRARY_JSON_MAX) {
        fclose(f); return LIBRARY_STORE_TOO_LARGE;
    }
    rewind(f);
    char *buf = (char *)malloc((size_t)file_size + 1);
    if (!buf) { fclose(f); return LIBRARY_STORE_IO_ERROR; }
    size_t got = fread(buf, 1, (size_t)file_size, f);
    int read_ok = !ferror(f);
    fclose(f);
    if (!read_ok || got != (size_t)file_size) { free(buf); return LIBRARY_STORE_IO_ERROR; }
    buf[got] = '\0';

    json_parser_t *parser = (json_parser_t *)malloc(sizeof(*parser));
    if (!parser) { free(buf); return LIBRARY_STORE_IO_ERROR; }
    size_t root, t, items;
    personal_library_t loaded;
    memset(&loaded, 0, sizeof(loaded));
    int ok = json_parse(parser, buf, got, &root) &&
        parser->tokens[root].type == JT_OBJECT &&
        object_string(parser, root, "library_id", loaded.library_id,
                      sizeof(loaded.library_id), 1) &&
        object_string(parser, root, "updated_at", loaded.updated_at,
                      sizeof(loaded.updated_at), 1) &&
        object_get(parser, root, "schema_version", &t) &&
        parser->tokens[t].type == JT_PRIMITIVE &&
        parser->tokens[t].end - parser->tokens[t].start == 1 &&
        parser->src[parser->tokens[t].start] == '1' &&
        object_get(parser, root, "items", &items) &&
        parser->tokens[items].type == JT_ARRAY;

    if (ok) {
        size_t i = items + 1;
        while (i < parser->tokens[items].next) {
            if (loaded.count >= PERSONAL_LIBRARY_MAX_ITEMS) {
                ok = 0; break;
            }
            if (!parse_item(parser, i, &loaded.items[loaded.count])) {
                ok = 0; break;
            }
            for (size_t k = 0; k < loaded.count; ++k) {
                if (strcmp(loaded.items[k].id, loaded.items[loaded.count].id) == 0)
                    ok = 0;
            }
            if (!ok) break;
            ++loaded.count;
            i = parser->tokens[i].next;
        }
    }
    if (ok) *library = loaded;
    free(parser);
    free(buf);
    return ok ? LIBRARY_STORE_OK : LIBRARY_STORE_INVALID;
}

static int write_json_string(FILE *f, const char *s) {
    if (fputc('"', f) == EOF) return 0;
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
        switch (*p) {
        case '"': if (fputs("\\\"", f) == EOF) return 0; break;
        case '\\': if (fputs("\\\\", f) == EOF) return 0; break;
        case '\b': if (fputs("\\b", f) == EOF) return 0; break;
        case '\f': if (fputs("\\f", f) == EOF) return 0; break;
        case '\n': if (fputs("\\n", f) == EOF) return 0; break;
        case '\r': if (fputs("\\r", f) == EOF) return 0; break;
        case '\t': if (fputs("\\t", f) == EOF) return 0; break;
        default:
            if (*p < 0x20) return 0;
            if (fputc(*p, f) == EOF) return 0;
        }
    }
    return fputc('"', f) != EOF;
}

#define WSTR(k, v) do { if (fputs("\"" k "\":", f) == EOF || !write_json_string(f, (v))) { ok = 0; goto save_cleanup; } } while (0)
#define WNUM(k, v) do { if (fprintf(f, "\"" k "\":%llu", (unsigned long long)(v)) < 0) { ok = 0; goto save_cleanup; } } while (0)

library_store_result_t personal_library_save(const char *path,
                                               const personal_library_t *library) {
    if (!path || !library || !library->library_id[0] ||
        library->count > PERSONAL_LIBRARY_MAX_ITEMS)
        return LIBRARY_STORE_INVALID;
    char tmp[PERSONAL_LIBRARY_PATH_MAX + 8];
    int n = snprintf(tmp, sizeof(tmp), "%s%s", path, LIBRARY_TMP_SUFFIX);
    if (n < 0 || (size_t)n >= sizeof(tmp)) return LIBRARY_STORE_INVALID;

    if (!library_ensure_parent(path)) return LIBRARY_STORE_IO_ERROR;
    FILE *f = fopen(tmp, "wb");
    if (!f) return LIBRARY_STORE_IO_ERROR;
    int ok = fputs("{\"schema_version\":1,", f) != EOF;
    if (ok) { WSTR("library_id", library->library_id); ok = fputc(',', f) != EOF; }
    if (ok) { WSTR("updated_at", library->updated_at); ok = fputs(",\"items\":[", f) != EOF; }

    for (size_t i = 0; ok && i < library->count; ++i) {
        const library_item_t *it = &library->items[i];
        const char *source_type = source_type_name(it->source.type);
        const char *status = library_item_status_name(it->status);
        if (!source_type || !status || !it->id[0] || !it->title[0] ||
            !it->created_at[0]) { ok = 0; break; }
        if (i && fputc(',', f) == EOF) { ok = 0; break; }
        if (fputc('{', f) == EOF) { ok = 0; break; }
        WSTR("id", it->id);
        if (fputc(',', f) == EOF) { ok = 0; break; }
        WSTR("title", it->title);
        if (it->platform[0]) { if (fputc(',', f) == EOF) { ok = 0; break; } WSTR("platform", it->platform); }
        if (fputs(",\"source\":{", f) == EOF) { ok = 0; break; }
        WSTR("type", source_type);
        if (fputc(',', f) == EOF) { ok = 0; break; }
        WSTR("uri", it->source.uri);
        if (fputs("},", f) == EOF) { ok = 0; break; }
        WSTR("status", status);
        if (fputc(',', f) == EOF) { ok = 0; break; }
        WSTR("created_at", it->created_at);
        if (it->target_uri[0]) {
            if (fputs(",\"target\":{", f) == EOF) { ok = 0; break; }
            int comma = 0;
            if (it->target_uri[0]) { WSTR("uri", it->target_uri); comma = 1; }
            if (it->target_platform[0]) { if (comma) fputc(',', f); WSTR("platform", it->target_platform); comma = 1; }
            if (it->target_module[0]) { if (comma) fputc(',', f); WSTR("module", it->target_module); }
            if (fputc('}', f) == EOF) { ok = 0; break; }
        }
        if (it->total_bytes || it->completed_bytes || it->info_hash[0] || it->transfer_verified) {
            if (fputs(",\"transfer\":{", f) == EOF) { ok = 0; break; }
            int comma = 0;
            if (it->info_hash[0]) { WSTR("info_hash", it->info_hash); comma = 1; }
            if (comma) fputc(',', f);
            WNUM("total_bytes", it->total_bytes);
            if (fputc(',', f) == EOF) { ok = 0; break; }
            WNUM("completed_bytes", it->completed_bytes);
            if (fprintf(f, ",\"verified\":%s}", it->transfer_verified ? "true" : "false") < 0) { ok = 0; break; }
        }
        if (fputc('}', f) == EOF) { ok = 0; break; }
    }
    if (ok && fputs("]}\n", f) == EOF) ok = 0;
save_cleanup:
    if (fflush(f) != 0) ok = 0;
    if (fclose(f) != 0) ok = 0;
    if (!ok) { remove(tmp); return LIBRARY_STORE_IO_ERROR; }
    if (rename(tmp, path) != 0) { remove(tmp); return LIBRARY_STORE_IO_ERROR; }
    return LIBRARY_STORE_OK;
}
