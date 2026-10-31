#include <borealis.hpp>
#include <switch.h>

#include <cstdlib>
#include <ctime>

extern "C" {
#include "core/util.h"
}

#include "library/import_manager.h"
#include "library/library_store.h"
#include "library/source_center.h"
#include "ui/main_view.hpp"

namespace {

constexpr const char* kLogPath = "sdmc:/retrotorrent.log";
constexpr const char* kLibraryPath =
    "sdmc:/switch/retrotorrent/library.json";
constexpr const char* kSourceRegistryPath = "romfs:/sources-v1.json";

std::string utcNow() {
    const std::time_t now = std::time(nullptr);
    std::tm value {};
    if (!gmtime_r(&now, &value))
        return "1970-01-01T00:00:00Z";

    char out[32] {};
    if (std::strftime(out, sizeof(out), "%Y-%m-%dT%H:%M:%SZ", &value) == 0)
        return "1970-01-01T00:00:00Z";
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    log_init(kLogPath);
    brls::Logger::setLogOutput(log_file());
    brls::Logger::setLogLevel(brls::LogLevel::LOG_INFO);

    source_center_registry_t sources {};
    const bool sourcesReady =
        source_center_load(kSourceRegistryPath, &sources) == SOURCE_CENTER_OK;

    personal_library_t library {};
    const std::string now = utcNow();
    personal_library_init(&library, "retrotorrent-personal", now.c_str());

    import_manager_t imports {};
    const bool libraryReady =
        import_manager_open(&imports, &library, kLibraryPath, now.c_str()) != 0;

    if (!sourcesReady)
        log_msg("[startup] source registry unavailable: %s\n",
                kSourceRegistryPath);
    if (!libraryReady)
        log_msg("[startup] personal library could not be opened: %s\n",
                kLibraryPath);

    brls::Platform::APP_LOCALE_DEFAULT = brls::LOCALE_RU;
    if (!brls::Application::init()) {
        log_msg("[startup] Borealis initialization failed\n");
        brls::Logger::setLogOutput(nullptr);
        log_close();
        return EXIT_FAILURE;
    }

    brls::Application::createWindow("RetroTorrent");
    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);
    brls::Application::setGlobalQuit(false);
    brls::Application::pushActivity(new brls::Activity(
        new retrotorrent::ui::MainView(
            &sources, &imports, &library, sourcesReady, libraryReady)));

    while (brls::Application::mainLoop()) {
    }

    brls::Logger::setLogOutput(nullptr);
    log_close();
    return EXIT_SUCCESS;
}
