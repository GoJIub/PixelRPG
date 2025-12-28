cmake_minimum_required(VERSION 3.10)
project(homework7)

# === Настройки C++ ===
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# === Флаги компиляции ===
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror=maybe-uninitialized")

# === Google Test через FetchContent ===
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.0
    TLS_VERIFY false
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# === Включение директорий ===
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# === SFML Configuration ===
# Try to find SFML
find_package(sfml-graphics QUIET)
find_package(sfml-window QUIET)
find_package(sfml-system QUIET)
find_package(sfml-audio QUIET)

if(sfml-graphics_FOUND AND sfml-window_FOUND AND sfml-system_FOUND)
    message(STATUS "SFML found")
else()
    message(WARNING "SFML not found. You may need to install SFML or set SFML_DIR.")
    # Set a variable to conditionally compile visual wrapper
    set(SFML_FOUND FALSE)
endif()

# === Основная программа ===
add_executable(${CMAKE_PROJECT_NAME}
    main.cpp
    src/npc.cpp
    src/orc.cpp
    src/squirrel.cpp
    src/bear.cpp
    src/druid.cpp
    src/game_utils.cpp
    src/visual_wrapper.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(sfml-graphics_FOUND AND sfml-window_FOUND AND sfml-system_FOUND)
    target_link_libraries(${CMAKE_PROJECT_NAME} sfml-graphics sfml-window sfml-system sfml-audio)
endif()

# === Тесты ===
enable_testing()

add_executable(tests
    test/tests.cpp
    src/npc.cpp
    src/orc.cpp
    src/squirrel.cpp
    src/bear.cpp
    src/druid.cpp
    src/game_utils.cpp
)

target_include_directories(tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tests
    gtest
    gtest_main
    pthread
)

# === Добавление тестов в ctest ===
add_test(NAME Homework7Tests COMMAND tests)
