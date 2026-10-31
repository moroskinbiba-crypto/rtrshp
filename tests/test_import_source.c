#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
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
    assert(source.uri[0] == '\0');
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


/* metainfo.c uses the project logger; the import-source unit test does not
   need the logger implementation. */
void log_msg(const char *fmt, ...) {
    (void)fmt;
}

static void test_torrent_file_metainfo_loading(void) {
    const char *path = "/tmp/rtrshp-import-source-test.torrent";
    FILE *file = fopen(path, "wb");
    assert(file);

    const char *prefix =
        "d4:infod6:lengthi1e4:name4:test12:piece lengthi16384e6:pieces20:";
    assert(fwrite(prefix, 1, strlen(prefix), file) == strlen(prefix));
    for (int i = 0; i < 20; ++i)
        assert(fputc('H', file) != EOF);
    assert(fwrite("ee", 1, 2, file) == 2);
    assert(fclose(file) == 0);

    library_import_source_t source;
    assert(library_import_source_set(&source, LIBRARY_IMPORT_TORRENT_FILE,
                                    path) == LIBRARY_IMPORT_SOURCE_OK);

    metainfo_t mi;
    assert(library_import_load_torrent(&source, &mi) == 1);
    assert(strcmp(mi.name, "test") == 0);
    assert(mi.num_files == 1);
    assert(mi.total_length == 1);
    metainfo_free(&mi);

    assert(remove(path) == 0);

    assert(library_import_source_set(&source, LIBRARY_IMPORT_LOCAL_FILE,
                                    "/tmp/not-a-torrent.rom") ==
           LIBRARY_IMPORT_SOURCE_OK);
    assert(library_import_load_torrent(&source, &mi) == 0);
}

int main(void) {
    test_local_paths();
    test_magnet_uris();
    test_bad_arguments_and_size();
    test_torrent_file_metainfo_loading();
    puts("import source tests passed");
    return 0;
}
