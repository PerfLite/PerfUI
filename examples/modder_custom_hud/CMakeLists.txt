cmake_minimum_required(VERSION 3.20)
project(ModderCustomHud VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find PerfUI SDK headers
find_path(PERFUI_INCLUDE_DIR "PerfUI/PerfUI.h"
    PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    "$ENV{PERFUI_SDK}/include"
    "$ENV{PERFUI_SDK}"
    ENV PERFUI_SDK
)

if (NOT PERFUI_INCLUDE_DIR)
    message(FATAL_ERROR "PerfUI SDK headers not found! Set PERFUI_SDK environment variable.")
endif()

# Find PerfUI Import Library (PerfUI.lib for dynamic linking against PerfUI.dll)
find_library(PERFUI_IMPORT_LIB
    NAMES PerfUI
    PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/windows/x64/release"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/Release"
    "$ENV{PERFUI_SDK}/lib"
    "$ENV{PERFUI_SDK}"
    ENV PERFUI_SDK
)

if (NOT PERFUI_IMPORT_LIB)
    message(FATAL_ERROR "PerfUI.lib import library not found! Build PerfUI or set PERFUI_SDK.")
endif()

add_library(ModderCustomHud SHARED
    CustomHealthBar.h
    ModEntry.cpp
)

target_include_directories(ModderCustomHud PRIVATE
    ${PERFUI_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link against the dynamic import library PerfUI.lib (mod does NOT get a static copy of Core)
target_link_libraries(ModderCustomHud PRIVATE
    ${PERFUI_IMPORT_LIB}
)

# Target output settings for Skyrim SKSE plugin
set_target_properties(ModderCustomHud PROPERTIES
    OUTPUT_NAME "ModderCustomHud"
    SUFFIX ".dll"
)
