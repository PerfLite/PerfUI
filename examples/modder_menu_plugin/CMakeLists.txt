cmake_minimum_required(VERSION 3.20)
project(ModderMenuPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_path(PERFUI_INCLUDE_DIR "PerfUI/PerfUI.h"
    PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
    "$ENV{PERFUI_SDK}/include"
    "$ENV{PERFUI_SDK}"
    ENV PERFUI_SDK
)

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

add_library(ModderMenuPlugin SHARED
    PluginMenu.h
    ModMenuEntry.cpp
)

target_include_directories(ModderMenuPlugin PRIVATE
    ${PERFUI_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (PERFUI_IMPORT_LIB)
    target_link_libraries(ModderMenuPlugin PRIVATE ${PERFUI_IMPORT_LIB})
endif()
