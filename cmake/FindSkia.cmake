# FindSkia.cmake
# Locates a pre-built Skia installation.
#
# Usage:
#   find_package(Skia REQUIRED)
#
# Variables consumed (set before find_package):
#   SKIA_DIR          — root of the Skia source tree (where include/ lives)
#   SKIA_BUILD_DIR    — where GN placed its output (e.g. out/Release)
#
# Variables produced:
#   Skia_FOUND
#   Skia::Skia           imported interface target

if (NOT SKIA_DIR)
    set(SKIA_DIR "$ENV{SKIA_DIR}" CACHE PATH "Skia source root")
endif()
if (NOT SKIA_BUILD_DIR)
    set(SKIA_BUILD_DIR "$ENV{SKIA_BUILD_DIR}" CACHE PATH "Skia GN build dir")
endif()

find_path(SKIA_INCLUDE_DIR
    NAMES include/core/SkCanvas.h
    HINTS "${SKIA_DIR}"
    DOC   "Skia source root (must contain include/)"
)

find_library(SKIA_LIBRARY
    NAMES skia
    HINTS "${SKIA_BUILD_DIR}" "${SKIA_DIR}/out/Release" "${SKIA_DIR}/out/Debug"
    DOC   "Path to libskia.a / skia.lib"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Skia
    REQUIRED_VARS SKIA_INCLUDE_DIR SKIA_LIBRARY)

if (Skia_FOUND AND NOT TARGET Skia::Skia)
    add_library(Skia::Skia STATIC IMPORTED)
    set_target_properties(Skia::Skia PROPERTIES
        IMPORTED_LOCATION             "${SKIA_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SKIA_INCLUDE_DIR}"
    )

    # Skia requires these system libs on Linux
    if (UNIX AND NOT APPLE)
        find_package(Threads REQUIRED)
        find_package(Freetype REQUIRED)
        find_package(FontConfig REQUIRED)
        set_property(TARGET Skia::Skia APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES
                Threads::Threads
                Freetype::Freetype
                Fontconfig::Fontconfig
                dl GL
        )
    elseif (APPLE)
        set_property(TARGET Skia::Skia APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES
                "-framework CoreFoundation"
                "-framework CoreGraphics"
                "-framework CoreText"
                "-framework Metal"
        )
    endif()
endif()

mark_as_advanced(SKIA_INCLUDE_DIR SKIA_LIBRARY)
