cmake_minimum_required(VERSION 3.11)

project(LakotUtilities LANGUAGES CXX)

include(FetchContent)

add_library(${PROJECT_NAME}
    FileManager.h FileManager.cpp
    Timer.h Timer.cpp
    Type.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
