cmake_minimum_required(VERSION 3.20)
project(ModderCustomHud VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# An external mod only needs to link against PerfUI_Core (or include PerfUIApi.h)
# and include the public headers directory: include/
find_path(PERFUI_INCLUDE_DIR "PerfUI/PerfUI.h"
    PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    ENV PERFUI_SDK
)

if (NOT PERFUI_INCLUDE_DIR)
    message(FATAL_ERROR "PerfUI SDK headers not found! Set PERFUI_SDK environment variable.")
endif()

add_library(ModderCustomHud SHARED
    CustomHealthBar.h
    ModEntry.cpp
)

target_include_directories(ModderCustomHud PRIVATE
    ${PERFUI_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Target output settings for Skyrim SKSE plugin
set_target_properties(ModderCustomHud PROPERTIES
    OUTPUT_NAME "ModderCustomHud"
    SUFFIX ".dll"
)
