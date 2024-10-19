cmake_minimum_required(VERSION 3.11)

project(LakotCore LANGUAGES CXX)

set(LAKOT_CORE_FILES
    engine/Engine.h engine/Engine.cpp
    engine/EngineFactory.h engine/EngineFactory.cpp

    engine/GarbageCollector.h engine/GarbageCollector.cpp
    engine/GarbageCollectorFactory.h engine/GarbageCollectorFactory.cpp

    engine/Logger.h engine/Logger.cpp

    geometry/BoundingBox.h geometry/BoundingBox.cpp

    graphics/api/GraphicsAPIFactory.h graphics/api/GraphicsAPIFactory.cpp
    graphics/api/GraphicsAPI.h graphics/api/GraphicsAPI.cpp

    graphics/window/WindowFactory.h graphics/window/WindowFactory.cpp
    graphics/window/Window.h graphics/window/Window.cpp

    graphics/camera/Camera.h graphics/camera/Camera.cpp
    graphics/camera/CameraFactory.h graphics/camera/CameraFactory.cpp
    graphics/camera/FPSCamera.h graphics/camera/FPSCamera.cpp

    layer/Layer.h layer/Layer.cpp
    layer/LayerFactory.h layer/LayerFactory.cpp

    render/RendererFactory.h render/RendererFactory.cpp

    resource/asset/BoneAsset.h resource/asset/BoneAsset.cpp
    resource/asset/ModelAsset.h resource/asset/ModelAsset.cpp
    resource/asset/NodeAsset.h resource/asset/NodeAsset.cpp
    resource/asset/MeshAsset.h resource/asset/MeshAsset.cpp
    resource/asset/MaterialAsset.h resource/asset/MaterialAsset.cpp
    resource/asset/TextureAsset.h resource/asset/TextureAsset.cpp

    resource/entity/Entity.h resource/entity/Entity.cpp

    resource/loader/ModelLoader.h resource/loader/ModelLoader.cpp
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

add_library(${PROJECT_NAME} ${LAKOT_CORE_FILES}
    scene/Scene.h scene/Scene.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC LakotAbstract)
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
    if (LAKOT_USE_PLATFORM_WINDOWS) # v3.4
        set(glfw3_DIR ${LAKOT_EXTERNAL_PATH}/glfw/windows/lib/cmake/glfw3)
        find_package(glfw3 REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
    else()
        message(FATAL_ERROR "Undefined platform to link glfw library.")
    endif()
endif()
### add glfw - end ###

### add glew - begin ###
if(LAKOT_USE_RENDERER_OPENGL)
    if (LAKOT_USE_PLATFORM_WINDOWS) # v2.2.0
        set(glew_DIR ${LAKOT_EXTERNAL_PATH}/glew/windows)
        set(glew_bin_DIR ${glew_DIR}/bin)
        set(glew_include_DIR ${glew_DIR}/include)
        set(glew_lib_DIR ${glew_DIR}/lib)

        target_include_directories(${PROJECT_NAME} PRIVATE ${glew_include_DIR})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${glew_lib_DIR}/glew32.lib)
        target_link_libraries(${PROJECT_NAME} PRIVATE opengl32.lib)
    else()
        message(FATAL_ERROR "Undefined platform to link glew library.")
    endif()
endif()
### add glew - end ###

### add assimp - begin ###
if (LAKOT_USE_PLATFORM_WINDOWS) # v5.4.3
    set(assimp_DIR ${LAKOT_EXTERNAL_PATH}/assimp/windows/lib/cmake/assimp-5.4)
    find_package(assimp REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)
elseif(LAKOT_USE_PLATFORM_ANDROID)
    set(assimp_DIR ${LAKOT_EXTERNAL_PATH}/assimp/android/lib/cmake/assimp-5.4)
    find_package(assimp REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)
else()
    message(FATAL_ERROR "Undefined platform to link assimp library.")
endif()
### add assimp - end ###

### add stb - begin ###
if (LAKOT_USE_PLATFORM_WINDOWS) # v5.4.3
    set(stb_include_DIR ${LAKOT_EXTERNAL_PATH}/stb)
    target_include_directories(${PROJECT_NAME} PRIVATE ${stb_include_DIR})
elseif(LAKOT_USE_PLATFORM_ANDROID)
    set(stb_include_DIR ${LAKOT_EXTERNAL_PATH}/stb)
    target_include_directories(${PROJECT_NAME} PRIVATE ${stb_include_DIR})
else()
    message(FATAL_ERROR "Undefined platform to link stb library.")
endif()
### add stb - end ###

### add boost - begin ###
if (LAKOT_USE_PLATFORM_WINDOWS) # v5.4.3
    set(Boost_DIR ${LAKOT_EXTERNAL_PATH}/boost/windows/lib/cmake/Boost-1.87.0)

    set(Boost_USE_STATIC_LIBS ON)  # Static linking istiyorsanız
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME OFF)

    find_package(Boost 1.87 CONFIG REQUIRED COMPONENTS asio)
    target_link_libraries(${PROJECT_NAME} PRIVATE Boost::asio)
elseif(LAKOT_USE_PLATFORM_ANDROID)
    set(assimp_DIR ${LAKOT_EXTERNAL_PATH}/assimp/android/lib/cmake/assimp-5.4)
    find_package(assimp REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)
else()
    message(FATAL_ERROR "Undefined platform to link assimp library.")
endif()
### add boost - end ###
