cmake_minimum_required(VERSION 3.20)
project(ModderMenuPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ModderMenuPlugin SHARED
    PluginMenu.h
    ModMenuEntry.cpp
)

target_include_directories(ModderMenuPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
