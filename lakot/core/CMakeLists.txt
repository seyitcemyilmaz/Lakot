cmake_minimum_required(VERSION 3.11)

project(LakotCore LANGUAGES CXX)

set(LAKOT_CORE_FILES
    engine/Engine.h engine/Engine.cpp
    engine/EngineFactory.h engine/EngineFactory.cpp

    engine/GarbageCollector.h engine/GarbageCollector.cpp
    engine/GarbageCollectorFactory.h engine/GarbageCollectorFactory.cpp

    engine/Logger.h engine/Logger.cpp

    graphics/api/GraphicsAPIFactory.h graphics/api/GraphicsAPIFactory.cpp
    graphics/api/GraphicsAPI.h graphics/api/GraphicsAPI.cpp

    graphics/camera/Camera.h graphics/camera/Camera.cpp
    graphics/camera/CameraFactory.h graphics/camera/CameraFactory.cpp
    graphics/camera/FPSCamera.h graphics/camera/FPSCamera.cpp

    graphics/geometry/Box.h graphics/geometry/Box.cpp

    graphics/window/WindowFactory.h graphics/window/WindowFactory.cpp
    graphics/window/Window.h graphics/window/Window.cpp

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

    scene/Scene.h scene/Scene.cpp
)

if(LAKOT_USE_RENDERER_OPENGL)
    list(APPEND LAKOT_CORE_FILES render/ShaderSource.h)

    list(APPEND LAKOT_CORE_FILES render/OpenGLRenderer.h render/OpenGLRenderer.cpp)

    list(APPEND LAKOT_CORE_FILES render/VertexArrayObject.h render/VertexArrayObject.cpp)
    list(APPEND LAKOT_CORE_FILES render/VertexBufferObject.h render/VertexBufferObject.cpp)

    list(APPEND LAKOT_CORE_FILES graphics/api/OpenGLAPI.h graphics/api/OpenGLAPI.cpp)

    list(APPEND LAKOT_CORE_FILES graphics/shader/OpenGLShader.h graphics/shader/OpenGLShader.cpp)
    list(APPEND LAKOT_CORE_FILES graphics/shader/OpenGLShaderProgram.h graphics/shader/OpenGLShaderProgram.cpp)
endif()

if(LAKOT_USE_WINDOW_GLFW)
    list(APPEND LAKOT_CORE_FILES graphics/window/WindowGLFW.h graphics/window/WindowGLFW.cpp)
endif()

if(LAKOT_USE_WINDOW_EGL)
    list(APPEND LAKOT_CORE_FILES graphics/window/WindowEGL.h graphics/window/WindowEGL.cpp)
endif()

add_library(${PROJECT_NAME} ${LAKOT_CORE_FILES})

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

if(LAKOT_USE_RENDERER_OPENGL)
    if (LAKOT_USE_PLATFORM_WINDOWS)
        if (LAKOT_USE_RENDERER_OPENGL_TYPE STREQUAL "Core")
            ### add glfw v3.4 - begin ###
            set(glfw3_DIR ${LAKOT_EXTERNAL_PATH}/glfw/windows/lib/cmake/glfw3)
            find_package(glfw3 REQUIRED)
            target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
            ### add glfw v3.4 - end ###

            ### add glew v2.2.0 - begin ###
            set(glew_DIR ${LAKOT_EXTERNAL_PATH}/glew/windows)
            set(glew_bin_DIR ${glew_DIR}/bin)
            set(glew_include_DIR ${glew_DIR}/include)
            set(glew_lib_DIR ${glew_DIR}/lib)

            target_include_directories(${PROJECT_NAME} PRIVATE ${glew_include_DIR})
            target_link_libraries(${PROJECT_NAME} PRIVATE ${glew_lib_DIR}/glew32.lib)
            target_link_libraries(${PROJECT_NAME} PRIVATE opengl32.lib)
            ### add glew v2.2.0 - end ###
        endif()

        # ### add boost - v1.87.0 - begin ###
        # set(Boost_DIR ${LAKOT_EXTERNAL_PATH}/boost/windows/lib/cmake/Boost-1.87.0)

        # set(Boost_USE_STATIC_LIBS ON)
        # set(Boost_USE_MULTITHREADED ON)
        # set(Boost_USE_STATIC_RUNTIME OFF)

        # find_package(Boost 1.87 CONFIG REQUIRED COMPONENTS asio)
        # target_link_libraries(${PROJECT_NAME} PRIVATE Boost::asio)
        # ### add boost - v1.87.0 - end ###

        ### add assimp v5.4.3 - begin ###
        set(assimp_DIR ${LAKOT_EXTERNAL_PATH}/assimp/windows/lib/cmake/assimp-5.4)
        find_package(assimp REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)
        ### add assimp v5.4.3 - end ###

        ### add stb - begin ###
        set(stb_include_DIR ${LAKOT_EXTERNAL_PATH}/stb)
        target_include_directories(${PROJECT_NAME} PRIVATE ${stb_include_DIR})
        ### add stb - end ###

    elseif(LAKOT_USE_PLATFORM_ANDROID)
        ### add assimp v5.4.3 - begin ###
        set(assimp_DIR ${LAKOT_EXTERNAL_PATH}/assimp/android/lib/cmake/assimp-5.4)
        find_package(assimp REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)
        ### add assimp v5.4.3 - end ###

        ### add stb - begin ###
        set(stb_include_DIR ${LAKOT_EXTERNAL_PATH}/stb)
        target_include_directories(${PROJECT_NAME} PRIVATE ${stb_include_DIR})
        ### add stb - end ###
    else()

    endif()
endif()
