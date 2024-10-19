cmake_minimum_required(VERSION 3.11)

project(LakotAbstract LANGUAGES CXX)

add_library(${PROJECT_NAME}
    engine/ALogger.h engine/ALogger.cpp

    graphics/api/AGraphicsAPI.h graphics/api/AGraphicsAPI.cpp
    graphics/window/AWindow.h graphics/window/AWindow.cpp
    graphics/camera/ACamera.h graphics/camera/ACamera.cpp

    render/ARenderer.h render/ARenderer.cpp
    render/ARenderable.h render/ARenderable.cpp

    layer/ALayer.h layer/ALayer.cpp

    resource/asset/AAsset.h resource/asset/AAsset.cpp

    resource/entity/AEntity.h resource/entity/AEntity.cpp
    scene/AScene.h scene/AScene.cpp


)

target_link_libraries(${PROJECT_NAME} PUBLIC LakotUtilities)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
