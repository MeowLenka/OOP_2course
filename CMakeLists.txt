cmake_minimum_required(VERSION 4.2)

project(Lab1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(lab1
    src/main.cpp
    src/csv_loader.cpp
)

target_include_directories(lab1 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)