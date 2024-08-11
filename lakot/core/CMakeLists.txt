cmake_minimum_required(VERSION 3.11)

project(LakotCore LANGUAGES CXX)

add_library(${PROJECT_NAME}
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

target_link_libraries(${PROJECT_NAME} PRIVATE LakotAbstract)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

### add glfw - begin ###
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY	https://github.com/glfw/glfw.git
        GIT_TAG 	7b6aead9fb88b3623e3b3725ebb42670cbe4c579 #v3.4
    )

    FetchContent_MakeAvailable(glfw)

    target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
endif()
### add glfw - end ###

### add glew - begin ###
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    FetchContent_Declare(
      glew
      GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
      GIT_TAG f456deace7b408655109aaeff71421ef2d3858c6 #v2.2.0
    )

    FetchContent_MakeAvailable(glew)

    if(TARGET GLEW::glew)
        target_link_libraries(${PROJECT_NAME} PRIVATE GLEW::glew)
    else()
        # GLEW kütüphanesini manuel olarak linkleyin
        target_include_directories(${PROJECT_NAME} PRIVATE ${glew_SOURCE_DIR}/include)

        set(GLEW_DEBUG_POSTFIX "")

        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(GLEW_DEBUG_POSTFIX "d")
        endif()

        target_link_libraries(${PROJECT_NAME} PRIVATE ${glew_BINARY_DIR}/lib/glew${GLEW_DEBUG_POSTFIX}.lib)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${glew_BINARY_DIR}/lib/glew-shared${GLEW_DEBUG_POSTFIX}.lib)
        target_link_libraries(${PROJECT_NAME} PRIVATE opengl32.lib)
    endif()

endif()

### add glew - end ###

### add spdlog - begin ###
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY	https://github.com/gabime/spdlog.git
    GIT_TAG 	27cb4c76708608465c413f6d0e6b8d99a4d84302 #v1.14.1
)

FetchContent_MakeAvailable(spdlog)

target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog)

### add spdlog - end ###

### add glm - begin ###
FetchContent_Declare(
        glm
        GIT_REPOSITORY	https://github.com/g-truc/glm.git
        GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #v1.0.1
)

FetchContent_MakeAvailable(glm)

target_link_libraries(${PROJECT_NAME} PUBLIC glm::glm)
### add glm - end ###
