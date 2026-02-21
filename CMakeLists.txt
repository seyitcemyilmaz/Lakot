cmake_minimum_required(VERSION 3.16)
project(Lakot)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/LakotSettings.cmake)

add_subdirectory(src/utils)

add_subdirectory(src/connection)

# add_subdirectory(src/client)
add_subdirectory(src/server)
add_subdirectory(src/game)
