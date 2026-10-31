#include "import_source.h"

#include <ctype.h>
#include <string.h>

static int has_control_char(const char *s) {
    for (; *s; ++s) {
        unsigned char c = (unsigned char)*s;
        if (c < 0x20 || c == 0x7f)
            return 1;
    }
    return 0;
}

static int is_device_path(const char *s) {
    const char *colon = strchr(s, ':');
    if (!colon || colon == s || colon[1] != '/')
        return 0;

    for (const char *p = s; p < colon; ++p) {
        unsigned char c = (unsigned char)*p;
        if (!isalnum(c) && c != '_' && c != '-')
            return 0;
    }
    return 1;
}

static int is_absolute_path(const char *s) {
    if (s[0] == '/' && s[1] != '/')
        return 1;

    if (isalpha((unsigned char)s[0]) && s[1] == ':' &&
        (s[2] == '/' || s[2] == '\\'))
        return 1;

    return is_device_path(s);
}

static int has_parent_component(const char *s) {
    const char *p = s;

    while (*p) {
        while (*p == '/' || *p == '\\')
            ++p;

        const char *start = p;
        while (*p && *p != '/' && *p != '\\')
            ++p;

        if ((size_t)(p - start) == 2 && start[0] == '.' && start[1] == '.')
            return 1;
    }
    return 0;
}

static int valid_path(const char *s) {
    if (!s || !s[0] || !is_absolute_path(s) || has_control_char(s))
        return 0;
    if (has_parent_component(s))
        return 0;
    return 1;
}

static int valid_hex_hash(const char *s, size_t len) {
    if (len != 40)
        return 0;
    for (size_t i = 0; i < len; ++i) {
        if (!isxdigit((unsigned char)s[i]))
            return 0;
    }
    return 1;
}

static int valid_base32_hash(const char *s, size_t len) {
    if (len != 32)
        return 0;
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)toupper((unsigned char)s[i]);
        if (!((c >= 'A' && c <= 'Z') || (c >= '2' && c <= '7')))
            return 0;
    }
    return 1;
}

static int starts_with_ci(const char *s, const char *prefix) {
    while (*prefix) {
        if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix))
            return 0;
        ++s;
        ++prefix;
    }
    return 1;
}

static int valid_magnet(const char *uri) {
    static const char prefix[] = "magnet:?";
    static const char xt_prefix[] = "xt=urn:btih:";

    if (!starts_with_ci(uri, prefix))
        return 0;

    const char *query = strchr(uri, '?');
    if (!query)
        return 0;
    ++query;

    while (*query) {
        const char *end = strchr(query, '&');
        size_t part_len = end ? (size_t)(end - query) : strlen(query);

        if (part_len >= sizeof(xt_prefix) - 1 &&
            starts_with_ci(query, xt_prefix)) {
            const char *hash = query + sizeof(xt_prefix) - 1;
            size_t hash_len = part_len - (sizeof(xt_prefix) - 1);
            if (valid_hex_hash(hash, hash_len) ||
                valid_base32_hash(hash, hash_len))
                return 1;
        }

        if (!end)
            break;
        query = end + 1;
    }

    return 0;
}


static int valid_rutracker_topic(const char *uri) {
    static const char prefix[] =
        "https://rutracker.org/forum/viewtopic.php?t=";
    if (strncmp(uri, prefix, sizeof(prefix) - 1) != 0)
        return 0;

    const char *p = uri + sizeof(prefix) - 1;
    if (!isdigit((unsigned char)*p) || *p == '0')
        return 0;
    while (isdigit((unsigned char)*p))
        ++p;

    if (*p == '\0')
        return 1;
    if (*p++ != '&')
        return 0;

    /* Permit additional numeric topic parameters only; block credentials,
       fragments, alternate hosts and control characters. */
    if (!*p)
        return 0;
    for (; *p; ++p) {
        unsigned char ch = (unsigned char)*p;
        if (!(isalnum(ch) || ch == '=' || ch == '&' || ch == '_' ||
              ch == '-' || ch == '.'))
            return 0;
    }
    return 1;
}

library_import_source_result_t library_import_source_set(
    library_import_source_t *out,
    library_import_source_type_t type,
    const char *uri) {
    if (out)
        memset(out, 0, sizeof(*out));
    if (!out || !uri)
        return LIBRARY_IMPORT_SOURCE_INVALID_ARGUMENT;

    size_t len = 0;
    while (len < LIBRARY_IMPORT_URI_MAX && uri[len])
        ++len;
    if (len == 0)
        return LIBRARY_IMPORT_SOURCE_INVALID_ARGUMENT;
    if (len == LIBRARY_IMPORT_URI_MAX)
        return LIBRARY_IMPORT_SOURCE_TOO_LONG;
    if (has_control_char(uri))
        return type == LIBRARY_IMPORT_MAGNET
            ? LIBRARY_IMPORT_SOURCE_INVALID_MAGNET
            : LIBRARY_IMPORT_SOURCE_INVALID_PATH;

    switch (type) {
    case LIBRARY_IMPORT_LOCAL_FILE:
    case LIBRARY_IMPORT_TORRENT_FILE:
        if (!valid_path(uri))
            return LIBRARY_IMPORT_SOURCE_INVALID_PATH;
        break;
    case LIBRARY_IMPORT_MAGNET:
        if (!valid_magnet(uri))
            return LIBRARY_IMPORT_SOURCE_INVALID_MAGNET;
        break;
    case LIBRARY_IMPORT_TRACKER_TOPIC:
        if (!valid_rutracker_topic(uri))
            return LIBRARY_IMPORT_SOURCE_INVALID_PATH;
        break;
    default:
        return LIBRARY_IMPORT_SOURCE_INVALID_TYPE;
    }

    out->type = type;
    memcpy(out->uri, uri, len + 1);
    return LIBRARY_IMPORT_SOURCE_OK;
}


int library_import_load_torrent(const library_import_source_t *source,
                                metainfo_t *out) {
    if (!source || !out)
        return 0;

    memset(out, 0, sizeof(*out));
    library_import_source_t checked;
    if (library_import_source_set(&checked, source->type, source->uri) !=
        LIBRARY_IMPORT_SOURCE_OK)
        return 0;
    if (checked.type != LIBRARY_IMPORT_TORRENT_FILE)
        return 0;
    return metainfo_load(checked.uri, out);
}

const char *library_import_source_result_string(
    library_import_source_result_t result) {
    switch (result) {
    case LIBRARY_IMPORT_SOURCE_OK:
        return "ok";
    case LIBRARY_IMPORT_SOURCE_INVALID_ARGUMENT:
        return "invalid argument";
    case LIBRARY_IMPORT_SOURCE_TOO_LONG:
        return "source URI is too long";
    case LIBRARY_IMPORT_SOURCE_INVALID_PATH:
        return "path must be absolute and must not contain parent-directory segments";
    case LIBRARY_IMPORT_SOURCE_INVALID_MAGNET:
        return "magnet URI must contain a valid xt=urn:btih BitTorrent info-hash";
    case LIBRARY_IMPORT_SOURCE_INVALID_TYPE:
        return "unsupported import source type";
    default:
        return "unknown import source error";
    }
}
