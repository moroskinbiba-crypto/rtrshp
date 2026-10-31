#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/library/import_source.h"

static void test_local_paths(void) {
    library_import_source_t source;

    assert(library_import_source_set(&source, LIBRARY_IMPORT_LOCAL_FILE,
                                    "sdmc:/roms/snes/game.sfc") ==
           LIBRARY_IMPORT_SOURCE_OK);
    assert(source.type == LIBRARY_IMPORT_LOCAL_FILE);
    assert(strcmp(source.uri, "sdmc:/roms/snes/game.sfc") == 0);

    assert(library_import_source_set(&source, LIBRARY_IMPORT_TORRENT_FILE,
                                    "usb:/torrents/library.torrent") ==
           LIBRARY_IMPORT_SOURCE_OK);

    assert(library_import_source_set(&source, LIBRARY_IMPORT_LOCAL_FILE,
                                    "sdmc:/roms/../private/game.sfc") ==
           LIBRARY_IMPORT_SOURCE_INVALID_PATH);
    assert(source.uri[0] == '\\0');
    assert(library_import_source_set(&source, LIBRARY_IMPORT_LOCAL_FILE, "") ==
           LIBRARY_IMPORT_SOURCE_INVALID_ARGUMENT);
    assert(library_import_source_set(&source, LIBRARY_IMPORT_LOCAL_FILE,
                                    "relative/game.sfc") ==
           LIBRARY_IMPORT_SOURCE_INVALID_PATH);
    assert(library_import_source_set(&source, LIBRARY_IMPORT_LOCAL_FILE,
                                    "sdmc:/roms/bad\nname.sfc") ==
           LIBRARY_IMPORT_SOURCE_INVALID_PATH);
}

static void test_magnet_uris(void) {
    library_import_source_t source;
    const char *hex =
        "magnet:?xt=urn:btih:0123456789abcdef0123456789abcdef01234567&dn=Example";
    const char *base32 =
        "magnet:?xt=urn:btih:ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

    assert(library_import_source_set(&source, LIBRARY_IMPORT_MAGNET, hex) ==
           LIBRARY_IMPORT_SOURCE_OK);
    assert(strcmp(source.uri, hex) == 0);
    assert(library_import_source_set(&source, LIBRARY_IMPORT_MAGNET, base32) ==
           LIBRARY_IMPORT_SOURCE_OK);
    assert(library_import_source_set(&source, LIBRARY_IMPORT_MAGNET,
                                    "magnet:?dn=missing-info-hash") ==
           LIBRARY_IMPORT_SOURCE_INVALID_MAGNET);
    assert(library_import_source_set(&source, LIBRARY_IMPORT_MAGNET,
                                    "magnet:?xt=urn:btih:xyz") ==
           LIBRARY_IMPORT_SOURCE_INVALID_MAGNET);
    assert(library_import_source_set(&source, LIBRARY_IMPORT_MAGNET,
                                    "https://example.invalid/file.torrent") ==
           LIBRARY_IMPORT_SOURCE_INVALID_MAGNET);
}

static void test_bad_arguments_and_size(void) {
    library_import_source_t source;
    char oversized[LIBRARY_IMPORT_URI_MAX + 1];

    memset(oversized, 'a', sizeof(oversized));
    oversized[sizeof(oversized) - 1] = '\0';

    assert(library_import_source_set(NULL, LIBRARY_IMPORT_MAGNET, "magnet:?") ==
           LIBRARY_IMPORT_SOURCE_INVALID_ARGUMENT);
    assert(library_import_source_set(&source, LIBRARY_IMPORT_MAGNET, NULL) ==
           LIBRARY_IMPORT_SOURCE_INVALID_ARGUMENT);
    assert(library_import_source_set(&source, LIBRARY_IMPORT_MAGNET, oversized) ==
           LIBRARY_IMPORT_SOURCE_TOO_LONG);
    assert(library_import_source_set(&source, (library_import_source_type_t)99,
                                    "anything") ==
           LIBRARY_IMPORT_SOURCE_INVALID_TYPE);
}

int main(void) {
    test_local_paths();
    test_magnet_uris();
    test_bad_arguments_and_size();
    puts("import source tests passed");
    return 0;
}
