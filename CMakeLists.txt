cmake_minimum_required(VERSION 4.2)

# --- Проект ---
project(csv_parser
    VERSION 1.0.0
    DESCRIPTION "CSV parser with C++20"
    LANGUAGES CXX
)

# --- Стандарт C++20 ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Опции ---
option(ENABLE_SANITIZERS "Enable Address/UB sanitizers in Debug" ON)

# --- Исполняемый файл ---
add_executable(csv_parser main.cpp)

# --- Директория с заголовками ---
target_include_directories(csv_parser PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# --- Предупреждения компилятора ---
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(csv_parser PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(csv_parser PRIVATE /W4)
endif()

# --- Санитайзеры только в Debug ---
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Sanitizers: ENABLED (Debug)")
        target_compile_options(csv_parser PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -fno-sanitize-recover=all
            -g
        )
        target_link_options(csv_parser PRIVATE
            -fsanitize=address,undefined
        )
    elseif(MSVC)
        message(STATUS "Sanitizers: ENABLED (MSVC AddressSanitizer)")
        target_compile_options(csv_parser PRIVATE /fsanitize=address)
        target_link_options(csv_parser PRIVATE /fsanitize=address)
    else()
        message(WARNING "Sanitizers not supported: ${CMAKE_CXX_COMPILER_ID}")
    endif()
else()
    message(STATUS "Sanitizers: DISABLED")
endif()

# --- Debug по умолчанию ---
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()