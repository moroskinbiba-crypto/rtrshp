cmake_minimum_required(VERSION 3.13)

# Borealis owns the Switch platform/toolchain setup. Keep the dependency pins
# in .gitmodules so a recursive checkout is reproducible.
set(BOREALIS_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/vendor/borealis/library)
set(PLATFORM_SWITCH ON CACHE BOOL "" FORCE)
set(USE_DEKO3D ON CACHE BOOL "" FORCE)
set(USE_GLFW OFF CACHE BOOL "" FORCE)
set(USE_SDL2 OFF CACHE BOOL "" FORCE)
set(USE_SYSTEM_FMT OFF CACHE BOOL "" FORCE)
set(USE_SYSTEM_TINYXML2 OFF CACHE BOOL "" FORCE)
set(USE_SYSTEM_TWEENY OFF CACHE BOOL "" FORCE)
set(BRLS_UNITY_BUILD OFF CACHE BOOL "" FORCE)

include(${BOREALIS_LIBRARY}/cmake/commonOption.cmake)
include(${BOREALIS_LIBRARY}/cmake/toolchain.cmake)

project(RetroTorrent VERSION 0.1.0 LANGUAGES C CXX)

set(APP_TITLE "RetroTorrent")
set(APP_AUTHOR "RetroTorrent")
set(ROMFS_DIR ${CMAKE_BINARY_DIR}/romfs)

if (PLATFORM_SWITCH)
    set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif ()

add_subdirectory(vendor/borealis/library)
target_include_directories(borealis PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm
)

# The first native app slice wires the existing personal-import backend to a
# Borealis screen. The BitTorrent engine/transfer queue is intentionally not
# linked into this stage yet.
set(APP_SOURCES
    src/main_switch.cpp
    src/library/import_source.c
    src/library/library_store.c
    src/library/import_manager.c
    src/library/source_center.c
    src/core/metainfo.c
    src/core/bencode.c
    src/core/sha1.c
    src/core/util.c
)

program_target(retrotorrent "${APP_SOURCES}")
set_target_properties(retrotorrent PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(retrotorrent PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/borealis/library/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm
)

target_compile_definitions(retrotorrent PRIVATE __SWITCH__)
target_compile_options(retrotorrent PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -ffunction-sections
    -fdata-sections
)
target_link_libraries(retrotorrent PRIVATE borealis)

# Borealis expects its default controls, images, materials and English locale
# under romfs. The source registry is bundled read-only; personal items remain
# in sdmc:/switch/retrotorrent/library.json.
file(MAKE_DIRECTORY ${ROMFS_DIR})
file(COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/borealis/resources/img/sys
    DESTINATION ${ROMFS_DIR}/img
)
file(COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/borealis/resources/material
    DESTINATION ${ROMFS_DIR}
)
file(COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/borealis/resources/i18n/en-US
    DESTINATION ${ROMFS_DIR}/i18n
)
add_custom_target(sync_sources_registry ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/catalog/sources-v1.json"
        "${ROMFS_DIR}/sources-v1.json"
    VERBATIM
)

find_program(NX_NACPTOOL_EXE NAMES nacptool
    HINTS "$ENV{DEVKITPRO}/tools/bin" "/opt/devkitpro/tools/bin")
find_program(NX_ELF2NRO_EXE NAMES elf2nro
    HINTS "$ENV{DEVKITPRO}/tools/bin" "/opt/devkitpro/tools/bin")
if (NOT NX_NACPTOOL_EXE OR NOT NX_ELF2NRO_EXE)
    message(FATAL_ERROR "devkitPro nacptool and elf2nro are required to package RetroTorrent.nro")
endif ()

set(RETROT_NACP ${CMAKE_BINARY_DIR}/retrotorrent.nacp)
set(RETROT_NRO ${CMAKE_BINARY_DIR}/switch/retrotorrent.nro)
add_custom_command(
    OUTPUT ${RETROT_NRO}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/switch"
    COMMAND ${NX_NACPTOOL_EXE}
        --create "RetroTorrent" "RetroTorrent" "${PROJECT_VERSION}" "${RETROT_NACP}"
    COMMAND ${NX_ELF2NRO_EXE}
        "$<TARGET_FILE:retrotorrent>" "${RETROT_NRO}"
        --nacp="${RETROT_NACP}"
        --romfsdir="${ROMFS_DIR}"
    DEPENDS retrotorrent sync_sources_registry
    VERBATIM
)
add_custom_target(retrotorrent_nro ALL DEPENDS ${RETROT_NRO})

add_custom_target(switch DEPENDS retrotorrent_nro)
