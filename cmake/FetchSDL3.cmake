# FetchSDL3.cmake — downloads and builds SDL3 via FetchContent if not found.

include(FetchContent)

# Try system package first
find_package(SDL3 QUIET CONFIG)
if (SDL3_FOUND)
    message(STATUS "swole: using system SDL3")
    return()
endif()

message(STATUS "swole: SDL3 not found on system — fetching from source")

FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.10   # update as needed
    GIT_SHALLOW    TRUE
)

set(SDL_SHARED         OFF CACHE BOOL "" FORCE)
set(SDL_STATIC         ON  CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY   OFF CACHE BOOL "" FORCE)
set(SDL_TESTS          OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_TESTS  OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(SDL3)
message(STATUS "swole: SDL3 fetched and configured")
