cmake_minimum_required(VERSION 3.11)

project(LakotAbstract LANGUAGES CXX)

add_library(${PROJECT_NAME}
    engine/AEngine.h engine/AEngine.cpp

    engine/ALogger.h engine/ALogger.cpp

    graphics/api/AGraphicsAPI.h graphics/api/AGraphicsAPI.cpp
    graphics/window/AWindow.h graphics/window/AWindow.cpp
    graphics/camera/ACamera.h graphics/camera/ACamera.cpp

    render/ARenderer.h render/ARenderer.cpp
    render/ARenderable.h render/ARenderable.cpp

    layer/ALayer.h layer/ALayer.cpp

    resource/asset/AAsset.h resource/asset/AAsset.cpp
    resource/asset/AModelAsset.h resource/asset/AModelAsset.cpp
    resource/asset/ANodeAsset.h resource/asset/ANodeAsset.cpp
    resource/asset/AMeshAsset.h resource/asset/AMeshAsset.cpp

    resource/entity/AEntity.h resource/entity/AEntity.cpp

    resource/loader/ALoader.h resource/loader/ALoader.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE LakotInterface)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
