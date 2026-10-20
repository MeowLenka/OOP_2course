cmake_minimum_required(VERSION 4.2)

project(Lab1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# По умолчанию - Debug (с санитайзерами).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Тип сборки" FORCE)
endif()

# Общие флаги: предупреждения + ASan/UBSan в Debug.
add_library(lab_options INTERFACE)
target_compile_options(lab_options INTERFACE
    -Wall -Wextra
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
    $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
)
target_link_options(lab_options INTERFACE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

add_library(lab_core STATIC
    src/csv_loader.cpp
    src/statistics.cpp
    src/histogram.cpp
    src/matrix.cpp
    src/preprocessing.cpp
)

target_include_directories(lab_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(lab_core PUBLIC lab_options)

add_executable(lab1 src/main.cpp)
target_link_libraries(lab1 PRIVATE lab_core)

# Утилита просмотра .bin (размеры + первые строки).
#add_executable(bin_viewer tools/bin_viewer.cpp)
#target_link_libraries(bin_viewer PRIVATE lab_core)


enable_testing()

function(add_lab_test name)
    add_executable(${name} tests/${name}.cpp)
    target_link_libraries(${name} PRIVATE lab_core)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_lab_test(test_matrix)
add_lab_test(test_csv_loader)
add_lab_test(test_statistics)
add_lab_test(test_preprocessing)

#cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#cmake --build build 
#./build/lab1
