cmake_minimum_required(VERSION 3.5)

project(LakotCore)

add_library(LakotCore
    engine/Engine.h engine/Engine.cpp
    engine/EngineFactory.h engine/EngineFactory.cpp

    engine/GarbageCollector.h engine/GarbageCollector.cpp
    engine/GarbageCollectorFactory.h engine/GarbageCollectorFactory.cpp

    graphics/api/GraphicsAPIFactory.h graphics/api/GraphicsAPIFactory.cpp
    graphics/api/GraphicsAPI.h graphics/api/GraphicsAPI.cpp
    graphics/api/OpenGLAPI.h graphics/api/OpenGLAPI.cpp

    graphics/window/WindowFactory.h graphics/window/WindowFactory.cpp
    graphics/window/Window.h graphics/window/Window.cpp
    graphics/window/WindowGLFW.h graphics/window/WindowGLFW.cpp

    graphics/camera/Camera.h graphics/camera/Camera.cpp
    graphics/camera/CameraFactory.h graphics/camera/CameraFactory.cpp
    graphics/camera/FPSCamera.h graphics/camera/FPSCamera.cpp

    layer/Layer.h layer/Layer.cpp
    render/Renderer.h render/Renderer.cpp
    layer/LayerFactory.h layer/LayerFactory.cpp
    render/RendererFactory.h render/RendererFactory.cpp
    render/OpenGLRenderer.h render/OpenGLRenderer.cpp
)

target_link_libraries(LakotCore PRIVATE LakotAbstract)
target_include_directories(LakotCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

### add glfw - begin ###
if (WIN32)
    find_package(glfw3 CONFIG REQUIRED)
    target_link_libraries(LakotCore PRIVATE glfw)
elseif (UNIX)

endif ()
### add glfw - end ###

### add glew - begin ###
if (WIN32)
    find_package(GLEW REQUIRED)
    target_link_libraries(LakotCore PRIVATE GLEW::GLEW)
elseif (UNIX)

endif()
### add glew - end ###

### add glm - begin ###
if (WIN32)
    find_package(glm CONFIG REQUIRED)
    target_link_libraries(LakotCore PRIVATE glm::glm)
elseif (UNIX)

endif()
### add glm - end ###

### add spdlog - begin ###
if (WIN32)
    find_package(spdlog CONFIG REQUIRED)
    target_link_libraries(LakotCore PRIVATE spdlog::spdlog)
elseif (UNIX)

endif()
### add spdlog - end ###
