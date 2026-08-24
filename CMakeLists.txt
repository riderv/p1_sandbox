cmake_minimum_required(VERSION 3.16)
# Переключаем проект на язык C++
project(p1_sandbox CXX)

# Заставляем CMake собирать финальный .exe прямо в корне проекта
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")


# Устанавливаем современный стандарт C++ (C++23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Просим CMake автоматически найти Raylib в системных путях MSYS2 UCRT64
find_package(raylib REQUIRED)

# 2. Объявляем исполняемый файл p1 и меняем main.c на main.cpp
add_executable(p1 src/main.cpp src/game.hpp src/pch.h src/talloc.c)
target_compile_options(p1 PRIVATE -fno-exceptions -fno-rtti)

# 3. Подключаем ваш предкомпилированный заголовок pch.h
target_precompile_headers(p1 PRIVATE src/pch.h)

# 4. Линкуем статическую библиотеку Raylib к проекту p1
target_link_libraries(p1 PRIVATE raylib)

# Помогает clangd гарантированно найти raylib.h
target_include_directories(p1 PRIVATE C:/msys64/ucrt64/include)
