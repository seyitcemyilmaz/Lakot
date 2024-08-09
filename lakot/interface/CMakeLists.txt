cmake_minimum_required(VERSION 3.5)

project(LakotInterface CXX)

add_library(LakotInterface INTERFACE
    engine/IEngine.h
    engine/IObject.h
    engine/IGarbageCollector.h

    graphics/api/IGraphicsAPI.h
    graphics/window/IWindow.h
    graphics/camera/ICamera.h

    graphics/render/IRenderer.h
    graphics/render/IRenderable.h

    layer/ILayer.h
)

target_include_directories(LakotInterface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../..)

### add glm - begin ###
if (WIN32)
    find_package(glm CONFIG REQUIRED)
    target_link_libraries(LakotInterface INTERFACE glm::glm)
elseif (UNIX)

endif()
### add glm - end ###
