cmake_minimum_required(VERSION 3.16)

# --- Windows vcpkg toolchain ---
if(WIN32)
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(PixelRPG LANGUAGES CXX)

# ------------------------------------------------------------
# Options
# ------------------------------------------------------------
option(PIXELRPG_HEADLESS "Build without SFML (no GUI)" OFF)

# ------------------------------------------------------------
# Target
# ------------------------------------------------------------
add_executable(PixelRPG)

# C++20
target_compile_features(PixelRPG PRIVATE cxx_std_20)

# Compile flags
if (MSVC)
    target_compile_options(PixelRPG PRIVATE /W4 /WX)
elseif (APPLE)
    target_compile_options(PixelRPG PRIVATE -Wall -Wextra -Werror=uninitialized)
else()
    target_compile_options(PixelRPG PRIVATE -Wall -Wextra -Werror=maybe-uninitialized)
endif()

# ------------------------------------------------------------
# Sources
# ------------------------------------------------------------
target_sources(PixelRPG PRIVATE
    main.cpp
    src/npc.cpp
    src/bear.cpp
    src/dragon.cpp
    src/druid.cpp
    src/orc.cpp
    src/squirrel.cpp
    src/game_utils.cpp
)

target_include_directories(PixelRPG PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ------------------------------------------------------------
# SFML (optional)
# ------------------------------------------------------------
if(NOT PIXELRPG_HEADLESS)
    message(STATUS "Building with SFML (GUI enabled)")

    # Разрешаем SFML версии 2.5 и выше
    find_package(SFML REQUIRED COMPONENTS graphics window system audio)

    # visual wrapper
    target_sources(PixelRPG PRIVATE src/visual_wrapper.cpp)

    target_link_libraries(PixelRPG PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
    )

    # Для Windows + vcpkg auto-link
    if(MSVC AND DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    endif()
else()
    message(STATUS "Building headless (no GUI)")
    target_compile_definitions(PixelRPG PRIVATE PIXELRPG_HEADLESS)
endif()
