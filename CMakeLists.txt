cmake_minimum_required(VERSION 3.16)

# --- Windows vcpkg toolchain ---
if(WIN32 AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
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

# ------------------------------------------------------------
# RPATH для Linux/macOS
# ------------------------------------------------------------
if(UNIX)
    set_target_properties(PixelRPG PROPERTIES
        BUILD_RPATH "$ORIGIN/lib"
        INSTALL_RPATH "$ORIGIN/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
        SKIP_BUILD_RPATH FALSE
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
endif()

# ------------------------------------------------------------
# Compile flags
# ------------------------------------------------------------
if(MSVC)
    target_compile_options(PixelRPG PRIVATE /W4)
else()
    target_compile_options(PixelRPG PRIVATE -Wall -Wextra -Werror=uninitialized)
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
    target_sources(PixelRPG PRIVATE src/visual_wrapper.cpp)

    if(UNIX AND NOT APPLE)
        find_package(SFML 2.5 REQUIRED COMPONENTS graphics window system audio)
        target_link_libraries(PixelRPG PRIVATE
            sfml-graphics
            sfml-window
            sfml-system
            sfml-audio
        )
        # ! Важно: НЕ линкуем libstb.so.0 на этапе сборки
    elseif(APPLE)
        # macOS: путь Homebrew
        if(EXISTS "/opt/homebrew/lib/cmake/SFML")
            set(SFML_DIR "/opt/homebrew/lib/cmake/SFML" CACHE PATH "")
        elseif(EXISTS "/usr/local/lib/cmake/SFML")
            set(SFML_DIR "/usr/local/lib/cmake/SFML" CACHE PATH "")
        endif()
        find_package(SFML 2 REQUIRED COMPONENTS graphics window system audio)
        target_link_libraries(PixelRPG PRIVATE
            sfml-graphics
            sfml-window
            sfml-system
            sfml-audio
        )
    else()
        # Windows + vcpkg
        find_package(SFML COMPONENTS Graphics Window System Audio QUIET)
        if(NOT SFML_FOUND)
            find_package(SFML REQUIRED COMPONENTS graphics window system audio)
        endif()
        target_link_libraries(PixelRPG PRIVATE
            sfml-graphics
            sfml-window
            sfml-system
            sfml-audio
        )
    endif()
else()
    message(STATUS "Building headless (no GUI)")
    target_compile_definitions(PixelRPG PRIVATE PIXELRPG_HEADLESS)
endif()