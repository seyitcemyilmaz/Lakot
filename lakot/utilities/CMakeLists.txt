cmake_minimum_required(VERSION 3.5)

project(LakotUtilities)

add_library(LakotUtilities
    FileManager.h FileManager.cpp
    Timer.h Timer.cpp
    Type.h
)

target_include_directories(LakotUtilities PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
