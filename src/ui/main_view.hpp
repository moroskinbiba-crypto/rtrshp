#pragma once

#include <borealis.hpp>
#include <borealis/views/scrolling_frame.hpp>
#include <switch.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <functional>
#include <string>

#include "library/import_manager.h"
#include "library/import_source.h"
#include "library/library_store.h"
#include "library/source_center.h"

namespace retrotorrent::ui {

class MainView final : public brls::Box {
public:
    MainView(source_center_registry_t* sources, import_manager_t* imports,
             personal_library_t* library, bool sourcesReady,
             bool libraryReady)
        : brls::Box(brls::Axis::COLUMN),
          sources_(sources),
          imports_(imports),
          library_(library),
          sourcesReady_(sourcesReady),
          libraryReady_(libraryReady) {
        setPadding(28.0f, 36.0f, 24.0f, 36.0f);
        setBackgroundColor(nvgRGB(17, 20, 27));

        auto* masthead = new brls::Box(brls::Axis::ROW);
        masthead->setAlignItems(brls::AlignItems::CENTER);
        masthead->setMarginBottom(22.0f);

        auto* brand = label("RetroTorrent", 32.0f, nvgRGB(245, 247, 250));
        masthead->addView(brand);

        auto* tagline = label("  РЕТРО-БИБЛИОТЕКА", 15.0f,
                              nvgRGB(135, 153, 176));
        tagline->setMarginTop(7.0f);
        masthead->addView(tagline);
        addView(masthead);

        auto* navigation = new brls::Box(brls::Axis::ROW);
        navigation->setMarginBottom(22.0f);
        navigation->setAlignItems(brls::AlignItems::CENTER);
        addNavButton(navigation, "Обзор", Page::Home);
        addNavButton(navigation, "Источники", Page::Sources);
        addNavButton(navigation, "Моя библиотека", Page::Library);
        addNavButton(navigation, "Настройки", Page::Settings);
        addView(navigation);

        contentFrame_ = new brls::ScrollingFrame();
        contentFrame_->setGrow(1.0f);
        contentFrame_->setCornerRadius(14.0f);
        contentFrame_->setBackgroundColor(nvgRGBA(39, 46, 58, 235));

        content_ = new brls::Box(brls::Axis::COLUMN);
        content_->setPadding(22.0f);
        content_->setGrow(1.0f);
        contentFrame_->setContentView(content_);
        addView(contentFrame_);

        showPage(Page::Home);
    }

private:
    enum class Page { Home, Sources, Library, Settings };

    static brls::Label* label(const std::string& value, float size,
                              NVGcolor color) {
        auto* result = new brls::Label();
        result->setText(value);
        result->setFontSize(size);
        result->setTextColor(color);
        return result;
    }

    void addText(const std::string& value, float size = 19.0f,
                 NVGcolor color = nvgRGB(225, 231, 239)) {
        auto* item = label(value, size, color);
        item->setMarginBottom(12.0f);
        content_->addView(item);
    }

    void addAction(const std::string& title, std::function<void()> callback,
                   bool primary = false) {
        auto* button = new brls::Button();
        button->setText(title);
        button->setFontSize(18.0f);
        button->setHeight(58.0f);
        button->setWidth(520.0f);
        button->setMarginBottom(12.0f);
        button->setStyle(primary ? &brls::BUTTONSTYLE_PRIMARY
                                 : &brls::BUTTONSTYLE_DEFAULT);
        button->registerClickAction(
            [callback = std::move(callback)](brls::View*) {
                callback();
                return true;
            });
        content_->addView(button);
    }

    void addNavButton(brls::Box* row, const std::string& title, Page page) {
        auto* button = new brls::Button();
        button->setText(title);
        button->setFontSize(17.0f);
        button->setHeight(54.0f);
        button->setWidth(210.0f);
        button->setMarginRight(10.0f);
        button->setStyle(page == currentPage_ ? &brls::BUTTONSTYLE_PRIMARY
                                              : &brls::BUTTONSTYLE_DEFAULT);
        button->registerClickAction([this, page](brls::View*) {
            showPage(page);
            return true;
        });
        navButtons_.push_back({button, page});
        row->addView(button);
    }

    void showPage(Page page) {
        currentPage_ = page;
        for (const auto& entry : navButtons_) {
            entry.first->setStyle(entry.second == currentPage_
                                      ? &brls::BUTTONSTYLE_PRIMARY
                                      : &brls::BUTTONSTYLE_DEFAULT);
        }
        content_->clearViews();

        switch (page) {
            case Page::Home: showHome(); break;
            case Page::Sources: showSources(); break;
            case Page::Library: showLibrary(); break;
            case Page::Settings: showSettings(); break;
        }
    }

    void showHome() {
        addText("Ваша коллекция игр — в одном месте", 27.0f,
                nvgRGB(246, 248, 252));
        addText(
            "Добавляйте собственные ROM-файлы, magnet-ссылки и .torrent. "
            "Записи сохраняются локально и остаются после перезапуска.",
            18.0f);
        addText("В библиотеке: " + std::to_string(library_->count) +
                    " записей",
                21.0f, nvgRGB(110, 214, 190));
        const auto* primary = sourcesReady_
                                  ? source_center_primary(sources_)
                                  : nullptr;
        addText(primary
                    ? "Основной источник: " + std::string(primary->title)
                    : "Основной источник не загружен",
                18.0f, primary ? nvgRGB(225, 231, 239)
                               : nvgRGB(255, 166, 120));
        if (!libraryReady_)
            addText("Не удалось открыть файл личной библиотеки. "
                    "Импорт сейчас недоступен.",
                    17.0f, nvgRGB(255, 166, 120));
        addAction("Добавить ROM-файл", [this] { addLocalFile(); }, true);
        addAction("Перейти к источникам", [this] {
            showPage(Page::Sources);
        });
        addAction("Открыть библиотеку", [this] {
            showPage(Page::Library);
        });
    }

    void showSources() {
        addText("Источники", 27.0f, nvgRGB(246, 248, 252));
        if (!sourcesReady_) {
            addText("Файл источников не найден или не прошёл проверку. "
                    "Личная библиотека при этом остаётся отдельной.",
                    18.0f, nvgRGB(255, 166, 120));
            return;
        }

        const auto* primary = source_center_primary(sources_);
        if (!primary) {
            addText("Не настроен основной источник.", 18.0f,
                    nvgRGB(255, 166, 120));
            return;
        }

        addText(std::string(primary->title) + "  ·  ОСНОВНОЙ ИСТОЧНИК", 24.0f,
                nvgRGB(110, 214, 190));
        addText(primary->homepage, 17.0f, nvgRGB(160, 185, 214));
        addText(primary->notes, 17.0f);
        addText("Доступные действия добавляют записи в личную библиотеку. "
                "Они пока не запускают загрузку.",
                16.0f, nvgRGB(175, 187, 204));

        if (primary->import_methods & SOURCE_IMPORT_TOPIC_REFERENCE)
            addAction("Сохранить ссылку на тему", [this] {
                saveTopicReference();
            }, true);
        if (primary->import_methods & SOURCE_IMPORT_TORRENT_FILE)
            addAction("Импортировать .torrent", [this] {
                importTorrentFile();
            });
        if (primary->import_methods & SOURCE_IMPORT_MAGNET_URI)
            addAction("Добавить magnet-ссылку", [this] {
                importMagnet();
            });
    }

    void showLibrary() {
        addText("Моя библиотека", 27.0f, nvgRGB(246, 248, 252));
        if (!libraryReady_) {
            addText("Файл библиотеки недоступен. Проверьте свободное место "
                    "на SD-карте и путь сохранения.",
                    18.0f, nvgRGB(255, 166, 120));
            return;
        }

        addText(std::to_string(library_->count) + " записей · "
                    "личные импорты не публикуются в общем каталоге",
                17.0f, nvgRGB(160, 185, 214));
        if (library_->count == 0) {
            addText("Пока пусто. Добавьте ROM-файл, magnet или .torrent.",
                    19.0f);
        } else {
            const size_t shown = std::min<size_t>(library_->count, 12);
            for (size_t offset = 0; offset < shown; ++offset) {
                const auto& item =
                    library_->items[library_->count - 1 - offset];
                const std::string platform =
                    item.platform[0] ? item.platform : "Платформа не указана";
                const std::string status = library_item_status_name(item.status)
                                               ? library_item_status_name(item.status)
                                               : "unknown";
                const std::string sourceType =
                    item.source.type == LIBRARY_IMPORT_LOCAL_FILE
                        ? "Локальный файл"
                        : item.source.type == LIBRARY_IMPORT_TORRENT_FILE
                              ? ".torrent"
                              : item.source.type == LIBRARY_IMPORT_MAGNET
                                    ? "Magnet"
                                    : "Ссылка на тему";
                addText(item.title + "\n" + platform + "  ·  " +
                            sourceType + "  ·  " + status,
                        17.0f);
            }
            if (library_->count > shown)
                addText("Показаны последние 12 записей. Всего: " +
                            std::to_string(library_->count),
                        15.0f, nvgRGB(160, 185, 214));
        }
        addAction("Добавить ROM-файл", [this] { addLocalFile(); }, true);
    }

    void showSettings() {
        addText("Настройки", 27.0f, nvgRGB(246, 248, 252));
        addText("Файл личной библиотеки", 18.0f,
                nvgRGB(110, 214, 190));
        addText("sdmc:/switch/retrotorrent/library.json", 16.0f,
                nvgRGB(160, 185, 214));
        addText("Реестр источников", 18.0f,
                nvgRGB(110, 214, 190));
        addText("romfs:/sources-v1.json", 16.0f,
                nvgRGB(160, 185, 214));
        addText("В этом этапе настройки ограничены путями хранения. "
                "Выбор каталогов tico, обложки и параметры движка будут "
                "подключаться отдельными этапами.",
                17.0f);
    }

    static bool askText(const char* guide, char* output, size_t capacity) {
        if (!output || capacity < 2)
            return false;
        output[0] = '\0';

        SwkbdConfig keyboard {};
        if (R_FAILED(swkbdCreate(&keyboard, 0)))
            return false;
        swkbdConfigMakePresetDefault(&keyboard);
        swkbdConfigSetGuideText(&keyboard, guide);
        swkbdConfigSetStringLenMax(
            &keyboard, static_cast<u32>(std::min<size_t>(capacity - 1, 500)));
        const Result result = swkbdShow(&keyboard, output, capacity);
        swkbdClose(&keyboard);
        return R_SUCCEEDED(result) && output[0] != '\0';
    }

    std::string newItemId(const char* prefix) {
        char value[96] {};
        const auto now = static_cast<unsigned long long>(
            std::time(nullptr));
        std::snprintf(value, sizeof(value), "%s-%llu-%u", prefix, now,
                      ++idSequence_);
        return value;
    }

    static std::string timestamp() {
        const std::time_t now = std::time(nullptr);
        std::tm value {};
        if (!gmtime_r(&now, &value))
            return "1970-01-01T00:00:00Z";
        char out[32] {};
        if (std::strftime(out, sizeof(out), "%Y-%m-%dT%H:%M:%SZ",
                          &value) == 0)
            return "1970-01-01T00:00:00Z";
        return out;
    }

    const source_center_source_t* primarySource() const {
        return sourcesReady_ ? source_center_primary(sources_) : nullptr;
    }

    void reportImport(import_manager_result_t result) {
        if (result == IMPORT_MANAGER_OK) {
            brls::Application::notify("Добавлено в личную библиотеку");
            showPage(Page::Library);
            return;
        }
        const char* reason = import_manager_result_string(result);
        brls::Application::notify(
            std::string("Не удалось добавить запись: ") +
            (reason ? reason : "неизвестная ошибка"));
    }

    void saveTopicReference() {
        if (!libraryReady_ || !imports_) {
            brls::Application::notify("Личная библиотека недоступна");
            return;
        }
        const auto* source = primarySource();
        if (!source) {
            brls::Application::notify("Источник не загружен");
            return;
        }

        char title[PERSONAL_LIBRARY_TITLE_MAX] {};
        char url[LIBRARY_IMPORT_URI_MAX] {};
        if (!askText("Название игры или раздачи", title, sizeof(title)) ||
            !askText("Ссылка RuTracker на тему", url, sizeof(url)))
            return;

        const std::string id = newItemId("topic");
        const std::string now = timestamp();
        reportImport(source_center_save_topic_reference(
            sources_, imports_, source->id, id.c_str(), title, "unknown",
            url, now.c_str()));
    }

    void importTorrentFile() {
        if (!libraryReady_ || !imports_) {
            brls::Application::notify("Личная библиотека недоступна");
            return;
        }
        const auto* source = primarySource();
        if (!source) {
            brls::Application::notify("Источник не загружен");
            return;
        }

        char title[PERSONAL_LIBRARY_TITLE_MAX] {};
        char path[LIBRARY_IMPORT_URI_MAX] {};
        if (!askText("Название записи", title, sizeof(title)) ||
            !askText("Полный путь к .torrent на SD или USB", path,
                     sizeof(path)))
            return;

        const std::string id = newItemId("torrent");
        const std::string now = timestamp();
        reportImport(source_center_import_torrent_file(
            sources_, imports_, source->id, id.c_str(), title, "unknown",
            path, now.c_str()));
    }

    void importMagnet() {
        if (!libraryReady_ || !imports_) {
            brls::Application::notify("Личная библиотека недоступна");
            return;
        }
        const auto* source = primarySource();
        if (!source) {
            brls::Application::notify("Источник не загружен");
            return;
        }

        char title[PERSONAL_LIBRARY_TITLE_MAX] {};
        char magnet[LIBRARY_IMPORT_URI_MAX] {};
        if (!askText("Название игры или раздачи", title, sizeof(title)) ||
            !askText("Magnet URI", magnet, sizeof(magnet)))
            return;

        const std::string id = newItemId("magnet");
        const std::string now = timestamp();
        reportImport(source_center_import_magnet_uri(
            sources_, imports_, source->id, id.c_str(), title, "unknown",
            magnet, now.c_str()));
    }

    void addLocalFile() {
        if (!libraryReady_ || !imports_) {
            brls::Application::notify("Личная библиотека недоступна");
            return;
        }

        char title[PERSONAL_LIBRARY_TITLE_MAX] {};
        char path[LIBRARY_IMPORT_URI_MAX] {};
        if (!askText("Название игры", title, sizeof(title)) ||
            !askText("Полный путь к ROM-файлу", path, sizeof(path)))
            return;

        library_import_source_t source {};
        const auto validation = library_import_source_set(
            &source, LIBRARY_IMPORT_LOCAL_FILE, path);
        if (validation != LIBRARY_IMPORT_SOURCE_OK) {
            brls::Application::notify(
                library_import_source_result_string(validation));
            return;
        }

        const std::string id = newItemId("local");
        const std::string now = timestamp();
        reportImport(import_manager_add(imports_, id.c_str(), title, "unknown",
                                        &source, now.c_str()));
    }

    source_center_registry_t* sources_ = nullptr;
    import_manager_t* imports_ = nullptr;
    personal_library_t* library_ = nullptr;
    bool sourcesReady_ = false;
    bool libraryReady_ = false;
    Page currentPage_ = Page::Home;
    brls::ScrollingFrame* contentFrame_ = nullptr;
    brls::Box* content_ = nullptr;
    std::vector<std::pair<brls::Button*, Page>> navButtons_;
    uint32_t idSequence_ = 0;
};

}  // namespace retrotorrent::ui
