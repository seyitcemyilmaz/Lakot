cmake_minimum_required(VERSION 3.11)

project(LakotInterface LANGUAGES CXX)

include(FetchContent)

add_library(${PROJECT_NAME} INTERFACE
    engine/IEngine.h
    engine/IGarbageCollector.h
    engine/ILogger.h
    engine/IObject.h

    graphics/api/IGraphicsAPI.h
    graphics/window/IWindow.h
    graphics/camera/ICamera.h

    graphics/render/IRenderer.h
    graphics/render/IRenderable.h

    layer/ILayer.h
)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../..)

### add glm - begin
FetchContent_Declare(
        glm
        GIT_REPOSITORY	https://github.com/g-truc/glm.git
        GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #v1.0.1
)

FetchContent_MakeAvailable(glm)

target_link_libraries(${PROJECT_NAME} INTERFACE glm::glm)
### add glm - end
