cmake_minimum_required(VERSION 3.11)

option(LAKOT_USE_PLATFORM_WINDOWS "Windows Platform" OFF)
option(LAKOT_USE_PLATFORM_LINUX "Linux Platform" OFF)
option(LAKOT_USE_PLATFORM_ANDROID "Android Platform" OFF)

option(LAKOT_USE_RENDERER_OPENGL "Use OpenGL" OFF)
option(LAKOT_USE_RENDERER_OPENGLES "Use OpenGLES" OFF)

option(LAKOT_USE_WINDOW_GLFW "Use GLFW" OFF)
option(LAKOT_USE_WINDOW_EGL "Use EGL" OFF)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(-DLAKOT_PLATFORM_WINDOWS)
    set(LAKOT_USE_PLATFORM_WINDOWS ON)
    set(LAKOT_USE_RENDERER_OPENGL ON)
    set(LAKOT_USE_WINDOW_GLFW ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-DLAKOT_PLATFORM_LINUX)
    set(LAKOT_USE_PLATFORM_LINUX ON)
    set(LAKOT_USE_RENDERER_OPENGL ON)
    set(LAKOT_USE_WINDOW_GLFW ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_definitions(-DLAKOT_PLATFORM_ANDROID)
    set(LAKOT_USE_PLATFORM_ANDROID ON)
    set(LAKOT_USE_RENDERER_OPENGLES ON)
    set(LAKOT_USE_WINDOW_EGL ON)
endif()

if(LAKOT_USE_RENDERER_OPENGL)
    add_definitions(-DLAKOT_RENDERER_OPENGL)
elseif(LAKOT_USE_RENDERER_OPENGLES)
    add_definitions(-DLAKOT_RENDERER_OPENGLES)
else()
    message(FATAL_ERROR "Renderer is not found.")
endif()

if(LAKOT_USE_WINDOW_GLFW)
    add_definitions(-DLAKOT_WINDOW_GLFW)
elseif(LAKOT_USE_WINDOW_EGL)
    add_definitions(-DLAKOT_WINDOW_EGL)
else()
    message(FATAL_ERROR "Renderer is not found.")
endif()

project(LakotCore LANGUAGES CXX)

set(LAKOT_CORE_FILES
    engine/Engine.h engine/Engine.cpp
    engine/EngineFactory.h engine/EngineFactory.cpp

    engine/GarbageCollector.h engine/GarbageCollector.cpp
    engine/GarbageCollectorFactory.h engine/GarbageCollectorFactory.cpp

    engine/Logger.h engine/Logger.cpp

    graphics/api/GraphicsAPIFactory.h graphics/api/GraphicsAPIFactory.cpp
    graphics/api/GraphicsAPI.h graphics/api/GraphicsAPI.cpp

    graphics/window/WindowFactory.h graphics/window/WindowFactory.cpp
    graphics/window/Window.h graphics/window/Window.cpp

    graphics/camera/Camera.h graphics/camera/Camera.cpp
    graphics/camera/CameraFactory.h graphics/camera/CameraFactory.cpp
    graphics/camera/FPSCamera.h graphics/camera/FPSCamera.cpp

    layer/Layer.h layer/Layer.cpp
    layer/LayerFactory.h layer/LayerFactory.cpp

    render/Renderer.h render/Renderer.cpp
    render/RendererFactory.h render/RendererFactory.cpp
)

if(LAKOT_USE_RENDERER_OPENGL)
    list(APPEND LAKOT_CORE_FILES graphics/api/OpenGLAPI.h graphics/api/OpenGLAPI.cpp)
    list(APPEND LAKOT_CORE_FILES render/OpenGLRenderer.h render/OpenGLRenderer.cpp)
endif()

if(LAKOT_USE_RENDERER_OPENGLES)
    list(APPEND LAKOT_CORE_FILES graphics/api/OpenGLESAPI.h graphics/api/OpenGLESAPI.cpp)
    list(APPEND LAKOT_CORE_FILES render/OpenGLESRenderer.h render/OpenGLESRenderer.cpp)
endif()

if(LAKOT_USE_WINDOW_GLFW)
    list(APPEND LAKOT_CORE_FILES graphics/window/WindowGLFW.h graphics/window/WindowGLFW.cpp)
endif()

if(LAKOT_USE_WINDOW_EGL)
    list(APPEND LAKOT_CORE_FILES graphics/window/WindowEGL.h graphics/window/WindowEGL.cpp)
endif()

add_library(${PROJECT_NAME} ${LAKOT_CORE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE LakotAbstract)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

if(LAKOT_USE_PLATFORM_ANDROID)
    find_package(game-activity REQUIRED CONFIG)
    target_link_libraries(${PROJECT_NAME} PRIVATE
            game-activity::game-activity
            EGL
            GLESv3
            jnigraphics
            android
    )
endif()

### add glfw - begin ###
if(LAKOT_USE_RENDERER_OPENGL)
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
if(LAKOT_USE_RENDERER_OPENGL)
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

        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
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
