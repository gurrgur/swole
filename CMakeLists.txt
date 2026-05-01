cmake_minimum_required(VERSION 3.25)

project(swole
    VERSION     0.1.0
    DESCRIPTION "SDL3 + Skia UI framework — modern C++ SWELL replacement"
    LANGUAGES   CXX
)

# ── Global settings ──────────────────────────────────────────────────────────

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ── Dependencies ─────────────────────────────────────────────────────────────

# SDL3 — fetched automatically if not installed
include(FetchSDL3)

# Skia — must be built separately (see docs/building-skia.md).
# Point SKIA_DIR and SKIA_BUILD_DIR at your build, or set them in a toolchain file.
find_package(Skia REQUIRED)

# ── SWELL compatibility objects ───────────────────────────────────────────────

add_library(swole_swell_core OBJECT
    src/swell/swell_core.cpp
)

set_target_properties(swole_swell_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(swole_swell_core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ── Library: swole ───────────────────────────────────────────────────────────

add_library(swole STATIC
    # core
    src/core/application.cpp

    # render
    src/render/canvas.cpp
    src/render/font.cpp
    src/render/image.cpp
    src/render/paint.cpp

    # window
    src/window/widget.cpp
    src/window/window.cpp

    # widgets
    src/widgets/button.cpp
    src/widgets/combo_box.cpp
    src/widgets/label.cpp
    src/widgets/list_box.cpp
    src/widgets/list_view.cpp
    src/widgets/progress_bar.cpp
    src/widgets/scroll_area.cpp
    src/widgets/slider.cpp
    src/widgets/tab_widget.cpp
    src/widgets/text_edit.cpp
    src/widgets/tree_view.cpp

    # menu & dialogs
    src/menu.cpp
    src/dialog.cpp

    # layout
    src/layout/layout.cpp
    src/layout/box_layout.cpp
    src/layout/form_layout.cpp
    src/layout/grid_layout.cpp

    # extra widgets
    src/widgets/group_box.cpp
    src/widgets/spin_box.cpp
    src/widgets/splitter.cpp
    src/widgets/status_bar.cpp
    src/widgets/tool_bar.cpp
)

add_library(swole::swole ALIAS swole)

target_include_directories(swole
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(swole
    PUBLIC
        Skia::Skia
        SDL3::SDL3
)

target_compile_options(swole PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /permissive-
    >
)

# Enable LTO in Release
set_target_properties(swole PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
)

# ── Examples ─────────────────────────────────────────────────────────────────

option(SWOLE_BUILD_EXAMPLES "Build example programs" ON)

if (SWOLE_BUILD_EXAMPLES)
    add_subdirectory(examples/hello)
endif()

# ── Install ───────────────────────────────────────────────────────────────────

include(GNUInstallDirs)

install(TARGETS swole
    EXPORT  swoleTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(EXPORT swoleTargets
    FILE      swoleTargets.cmake
    NAMESPACE swole::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/swole
)
