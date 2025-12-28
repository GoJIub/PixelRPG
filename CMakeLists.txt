cmake_minimum_required(VERSION 3.10)
project(PixelRPG)

# === Настройки C++ ===
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# === Флаги компиляции ===
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror=maybe-uninitialized")

# === Основная программа (СОЗДАЁМ ЦЕЛЬ СНАЧАЛА!) ===
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

# === Включение директорий ===
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# === SFML (ЛИНКУЕМ ПОСЛЕ СОЗДАНИЯ ЦЕЛИ) ===
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
)

# Опционально: сообщение, чтобы знать, что всё ок
message(STATUS "Linking with system SFML libraries")