#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "../src/library/import_source.h"
#include "../src/library/import_manager.h"
#include "../src/library/source_center.h"


void hex20(char out[41], const uint8_t hash[20]) {
    static const char digits[] = "0123456789abcdef";
    for (int i = 0; i < 20; ++i) {
        out[i * 2] = digits[hash[i] >> 4];
        out[i * 2 + 1] = digits[hash[i] & 15];
    }
    out[40] = '\0';
}

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


static void test_rutracker_topic_references(void) {
    library_import_source_t source;
    const char *topic =
        "https://rutracker.org/forum/viewtopic.php?t=1234567&start=20";
    assert(library_import_source_set(&source, LIBRARY_IMPORT_TRACKER_TOPIC,
                                    topic) == LIBRARY_IMPORT_SOURCE_OK);
    assert(source.type == LIBRARY_IMPORT_TRACKER_TOPIC);
    assert(strcmp(source.uri, topic) == 0);

    assert(library_import_source_set(&source, LIBRARY_IMPORT_TRACKER_TOPIC,
        "http://rutracker.org/forum/viewtopic.php?t=1234567") ==
        LIBRARY_IMPORT_SOURCE_INVALID_PATH);
    assert(library_import_source_set(&source, LIBRARY_IMPORT_TRACKER_TOPIC,
        "https://rutracker.org.evil.invalid/forum/viewtopic.php?t=1234567") ==
        LIBRARY_IMPORT_SOURCE_INVALID_PATH);
    assert(library_import_source_set(&source, LIBRARY_IMPORT_TRACKER_TOPIC,
        "https://rutracker.org/forum/viewtopic.php?t=0") ==
        LIBRARY_IMPORT_SOURCE_INVALID_PATH);
    assert(library_import_source_set(&source, LIBRARY_IMPORT_TRACKER_TOPIC,
        "https://rutracker.org/forum/viewtopic.php?t=abc") ==
        LIBRARY_IMPORT_SOURCE_INVALID_PATH);
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

    /* Importing a .torrent into the personal library records its info-hash
       and expected size before a real transfer queue is attached. */
    const char *library_path = "/tmp/retrotorrent-torrent-import-library.json";
    remove(library_path);
    personal_library_t library;
    import_manager_t manager;
    assert(import_manager_open(&manager, &library, library_path,
                               "2026-10-31T19:10:00Z"));
    assert(import_manager_add(&manager, "torrent-import-001", "Test torrent",
                              "snes", &source, "2026-10-31T19:11:00Z") ==
           IMPORT_MANAGER_OK);
    library_item_t *imported = personal_library_find(&library, "torrent-import-001");
    assert(imported);
    assert(strlen(imported->info_hash) == 40);
    assert(imported->total_bytes == 1);
    remove(library_path);

    assert(remove(path) == 0);

    assert(library_import_source_set(&source, LIBRARY_IMPORT_LOCAL_FILE,
                                    "/tmp/not-a-torrent.rom") ==
           LIBRARY_IMPORT_SOURCE_OK);
    assert(library_import_load_torrent(&source, &mi) == 0);
}


static void test_library_persistence_and_manager(void) {
    const char *path = "/tmp/retrotorrent-library-persist-test/library.json";
    remove(path);
    remove("/tmp/retrotorrent-library-persist-test/library.json.tmp");

    personal_library_t library;
    import_manager_t manager;
    assert(import_manager_open(&manager, &library, path, "2026-10-31T19:00:00Z"));
    assert(library.count == 0);

    library_import_source_t source;
    assert(library_import_source_set(&source, LIBRARY_IMPORT_MAGNET,
        "magnet:?xt=urn:btih:0123456789abcdef0123456789abcdef01234567&dn=Test") ==
        LIBRARY_IMPORT_SOURCE_OK);
    assert(import_manager_add(&manager, "personal-test-001", "Test \"Game\"",
                              "snes", &source, "2026-10-31T19:01:00Z") ==
           IMPORT_MANAGER_OK);
    assert(library.count == 1);
    assert(import_manager_add(&manager, "personal-test-001", "Duplicate", "snes",
                              &source, "2026-10-31T19:01:00Z") ==
           IMPORT_MANAGER_DUPLICATE);

    assert(import_manager_set_target(&manager, "personal-test-001", "snes",
                                     "snes9x", "sdmc:/tico/custom/snes",
                                     "2026-10-31T19:02:00Z") == IMPORT_MANAGER_OK);
    assert(import_manager_set_status(&manager, "personal-test-001",
                                     LIBRARY_ITEM_QUEUED,
                                     "2026-10-31T19:03:00Z") == IMPORT_MANAGER_OK);
    assert(import_manager_set_status(&manager, "personal-test-001",
                                     LIBRARY_ITEM_DOWNLOADING,
                                     "2026-10-31T19:04:00Z") == IMPORT_MANAGER_OK);
    assert(import_manager_set_progress(&manager, "personal-test-001", 512, 1024, 0,
                                       "2026-10-31T19:05:00Z") == IMPORT_MANAGER_OK);

    /* Reopening simulates a process restart: active work becomes paused. */
    personal_library_t restored;
    import_manager_t manager2;
    assert(import_manager_open(&manager2, &restored, path, "2026-10-31T19:06:00Z"));
    assert(restored.count == 1);
    library_item_t *item = personal_library_find(&restored, "personal-test-001");
    assert(item);
    assert(strcmp(item->title, "Test \"Game\"") == 0);
    assert(strcmp(item->platform, "snes") == 0);
    assert(strcmp(item->target_uri, "sdmc:/tico/custom/snes") == 0);
    assert(strcmp(item->target_module, "snes9x") == 0);
    assert(item->status == LIBRARY_ITEM_PAUSED);
    assert(item->completed_bytes == 512);
    assert(item->total_bytes == 1024);
    assert(!item->transfer_verified);

    assert(import_manager_set_status(&manager2, "personal-test-001",
                                     LIBRARY_ITEM_QUEUED,
                                     "2026-10-31T19:07:00Z") == IMPORT_MANAGER_OK);
    assert(import_manager_set_status(&manager2, "personal-test-001",
                                     LIBRARY_ITEM_READY,
                                     "2026-10-31T19:08:00Z") == IMPORT_MANAGER_BAD_TRANSITION);
    assert(import_manager_remove(&manager2, "personal-test-001") == IMPORT_MANAGER_OK);
    assert(restored.count == 0);

    personal_library_t empty;
    assert(personal_library_load(path, &empty) == LIBRARY_STORE_OK);
    assert(empty.count == 0);
    remove(path);
#ifndef _WIN32
    assert(rmdir("/tmp/retrotorrent-library-persist-test") == 0);
#endif
}


static void test_rutracker_reference_persistence(void) {
    const char *path = "/tmp/retrotorrent-rutracker-reference.json";
    remove(path);

    personal_library_t library;
    import_manager_t manager;
    assert(import_manager_open(&manager, &library, path, "2026-10-31T20:00:00Z"));

    library_import_source_t source;
    assert(library_import_source_set(&source, LIBRARY_IMPORT_TRACKER_TOPIC,
        "https://rutracker.org/forum/viewtopic.php?t=7654321") ==
        LIBRARY_IMPORT_SOURCE_OK);
    assert(import_manager_add(&manager, "rutracker-ref-001", "Saved topic",
                              "psx", &source, "2026-10-31T20:01:00Z") ==
           IMPORT_MANAGER_OK);

    personal_library_t restored;
    assert(personal_library_load(path, &restored) == LIBRARY_STORE_OK);
    assert(restored.count == 1);
    library_item_t *saved = personal_library_find(&restored, "rutracker-ref-001");
    assert(saved);
    assert(saved->source.type == LIBRARY_IMPORT_TRACKER_TOPIC);
    assert(strcmp(saved->source.uri,
        "https://rutracker.org/forum/viewtopic.php?t=7654321") == 0);
    remove(path);
}


static void write_test_torrent(const char *path) {
    FILE *file = fopen(path, "wb");
    assert(file);
    const char *prefix =
        "d4:infod6:lengthi1e4:name4:test12:piece lengthi16384e6:pieces20:";
    assert(fwrite(prefix, 1, strlen(prefix), file) == strlen(prefix));
    for (int i = 0; i < 20; ++i)
        assert(fputc('H', file) != EOF);
    assert(fwrite("ee", 1, 2, file) == 2);
    assert(fclose(file) == 0);
}

static void test_source_center_registry_and_import_actions(void) {
    source_center_registry_t registry;
    assert(source_center_load("catalog/sources-v1.json", &registry) ==
           SOURCE_CENTER_OK);
    assert(source_center_count(&registry) == 1);

    const source_center_source_t *primary = source_center_primary(&registry);
    assert(primary);
    assert(strcmp(primary->id, "rutracker") == 0);
    assert(strcmp(primary->title, "RuTracker") == 0);
    assert(primary->is_primary);
    assert(primary->enabled);
    assert(primary->priority == 100);
    assert(strcmp(primary->homepage,
                  "https://rutracker.org/forum/index.php") == 0);
    assert(primary->import_methods & SOURCE_IMPORT_TOPIC_REFERENCE);
    assert(primary->import_methods & SOURCE_IMPORT_TORRENT_FILE);
    assert(primary->import_methods & SOURCE_IMPORT_MAGNET_URI);
    assert(source_center_at(&registry, 1) == NULL);
    assert(source_center_find(&registry, "missing") == NULL);

    const char *library_path =
        "/tmp/retrotorrent-source-center-test/library.json";
    remove(library_path);
    personal_library_t library;
    import_manager_t manager;
    assert(import_manager_open(&manager, &library, library_path,
                               "2026-10-31T20:30:00Z"));

    assert(source_center_save_topic_reference(
        &registry, &manager, "rutracker", "rutracker-topic-0001",
        "Saved RuTracker topic", "snes",
        "https://rutracker.org/forum/viewtopic.php?t=1234567",
        "2026-10-31T20:31:00Z") == IMPORT_MANAGER_OK);

    library_item_t *topic = personal_library_find(&library, "rutracker-topic-0001");
    assert(topic);
    assert(topic->source.type == LIBRARY_IMPORT_TRACKER_TOPIC);
    assert(topic->status == LIBRARY_ITEM_IMPORTED);

    assert(source_center_save_topic_reference(
        &registry, &manager, "rutracker", "rutracker-topic-bad",
        "Wrong host", "snes",
        "https://rutracker.org.evil.invalid/forum/viewtopic.php?t=1234567",
        "2026-10-31T20:32:00Z") == IMPORT_MANAGER_INVALID);

    assert(source_center_import_magnet_uri(
        &registry, &manager, "rutracker", "rutracker-magnet-0001",
        "User supplied magnet", "psx",
        "magnet:?xt=urn:btih:0123456789abcdef0123456789abcdef01234567&dn=Test",
        "2026-10-31T20:33:00Z") == IMPORT_MANAGER_OK);

    const char *torrent_path = "/tmp/retrotorrent-source-center.torrent";
    write_test_torrent(torrent_path);
    assert(source_center_import_torrent_file(
        &registry, &manager, "rutracker", "rutracker-torrent-0001",
        "User supplied torrent", "snes", torrent_path,
        "2026-10-31T20:34:00Z") == IMPORT_MANAGER_OK);

    library_item_t *torrent = personal_library_find(&library,
                                                     "rutracker-torrent-0001");
    assert(torrent);
    assert(torrent->source.type == LIBRARY_IMPORT_TORRENT_FILE);
    assert(strlen(torrent->info_hash) == 40);
    assert(torrent->total_bytes == 1);

    assert(import_manager_set_target(&manager, "rutracker-topic-0001",
                                     "snes", "snes9x",
                                     "sdmc:/tico/custom/snes",
                                     "2026-10-31T20:35:00Z") == IMPORT_MANAGER_OK);

    personal_library_t restored;
    assert(personal_library_load(library_path, &restored) == LIBRARY_STORE_OK);
    assert(restored.count == 3);
    topic = personal_library_find(&restored, "rutracker-topic-0001");
    assert(topic && topic->source.type == LIBRARY_IMPORT_TRACKER_TOPIC);
    assert(strcmp(topic->target_uri, "sdmc:/tico/custom/snes") == 0);

    remove(torrent_path);
    remove(library_path);
#ifndef _WIN32
    assert(rmdir("/tmp/retrotorrent-source-center-test") == 0);
#endif
}

static void test_library_rejects_invalid_json(void) {
    const char *path = "/tmp/retrotorrent-invalid-library.json";
    FILE *f = fopen(path, "wb");
    assert(f);
    assert(fputs("{not-json", f) >= 0);
    assert(fclose(f) == 0);
    personal_library_t library;
    assert(personal_library_load(path, &library) == LIBRARY_STORE_INVALID);
    remove(path);
}

int main(void) {
    test_local_paths();
    test_magnet_uris();
    test_rutracker_topic_references();
    test_bad_arguments_and_size();
    test_torrent_file_metainfo_loading();
    test_library_persistence_and_manager();
    test_rutracker_reference_persistence();
    test_library_rejects_invalid_json();
    test_source_center_registry_and_import_actions();
    puts("import source tests passed");
    return 0;
}
