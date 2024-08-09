cmake_minimum_required(VERSION 3.5)

project(LakotAbstract)

add_library(LakotAbstract
    engine/AEngine.h engine/AEngine.cpp
    engine/AGarbageCollector.h engine/AGarbageCollector.cpp

    graphics/api/AGraphicsAPI.h graphics/api/AGraphicsAPI.cpp
    graphics/window/AWindow.h graphics/window/AWindow.cpp
    graphics/camera/ACamera.h graphics/camera/ACamera.cpp

    layer/ALayer.h layer/ALayer.cpp
    render/ARenderer.h render/ARenderer.cpp
    render/ARenderable.h render/ARenderable.cpp
)

target_link_libraries(LakotAbstract PRIVATE LakotInterface)
target_include_directories(LakotAbstract PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
