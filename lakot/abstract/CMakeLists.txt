cmake_minimum_required(VERSION 3.11)

project(LakotAbstract LANGUAGES CXX)

add_library(${PROJECT_NAME}
    engine/AEngine.h engine/AEngine.cpp
    engine/AGarbageCollector.h engine/AGarbageCollector.cpp

    graphics/api/AGraphicsAPI.h graphics/api/AGraphicsAPI.cpp
    graphics/window/AWindow.h graphics/window/AWindow.cpp
    graphics/camera/ACamera.h graphics/camera/ACamera.cpp

    layer/ALayer.h layer/ALayer.cpp
    render/ARenderer.h render/ARenderer.cpp
    render/ARenderable.h render/ARenderable.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE LakotInterface)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
