cmake_minimum_required(VERSION 3.16)
project(Lakot)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/LakotSettings.cmake)

### Game data shared by server and client (maps, kingdoms), copied next to
### the executables on every build - an ALL target for the same reason as
### LakotGameAssets: editing only data must still redeploy it.
add_custom_target(LakotData ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data
    COMMENT "Copying Lakot data"
)

add_subdirectory(src/utils)

add_subdirectory(src/connection)

# add_subdirectory(src/client)
add_subdirectory(src/server)
add_subdirectory(src/game)

add_dependencies(LakotServer LakotData)
add_dependencies(LakotGame LakotData)
