cmake_minimum_required(VERSION 3.16)

project(LakotGame LANGUAGES CXX)

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
    main.cpp

    Engine.h Engine.cpp

    gui/GuiLayer.h
    gui/GuiLayer.cpp

    gui/Panel.h
    gui/PanelLoader.h gui/PanelLoader.cpp
    gui/panels/LoginPanel.h gui/panels/LoginPanel.cpp

    network/NetworkManager.h network/NetworkManager.cpp
    network/controller/AuthController.h network/controller/AuthController.cpp

    graphics/camera/CameraFactory.h graphics/camera/CameraFactory.cpp
    graphics/camera/Camera.h graphics/camera/Camera.cpp
    graphics/camera/FPSCamera.h graphics/camera/FPSCamera.cpp

    graphics/geometry/BoxContainer.h graphics/geometry/BoxContainer.cpp

    graphics/render/Renderable.h graphics/render/Renderable.cpp

    graphics/render/Shader.h graphics/render/Shader.cpp
    graphics/render/ShaderProgram.h graphics/render/ShaderProgram.cpp
    graphics/render/VertexBufferObject.h graphics/render/VertexBufferObject.cpp
    graphics/render/VertexArrayObject.h graphics/render/VertexArrayObject.cpp

    scene/Scene.h
    scene/SceneManager.h scene/SceneManager.cpp
    scene/LoginScene.h scene/LoginScene.cpp
    scene/WorldScene.h scene/WorldScene.cpp
    graphics/render/ShaderManager.h graphics/render/ShaderManager.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE LakotConnection)

### add glad - start
add_subdirectory(${LAKOT_EXTERNAL_PATH}/glad "${CMAKE_CURRENT_BINARY_DIR}/glad_build")
target_link_libraries(${PROJECT_NAME} PRIVATE glad)
### add glad - end

### add sdl3 - start
if (LAKOT_USE_PLATFORM_WINDOWS)
    if (LAKOT_USE_BUILD_TYPE_RELEASE)
        set(SDL3_SOURCE_DIR ${LAKOT_EXTERNAL_PATH}/SDL3/windows/release)
    elseif(LAKOT_USE_BUILD_TYPE_DEBUG)
        set(SDL3_SOURCE_DIR ${LAKOT_EXTERNAL_PATH}/SDL3/windows/debug)
    else()
        message(FATAL_ERROR "Undefined build type to link SDL3 library.")
    endif()

    set(SDL3_BIN_DIR ${SDL3_SOURCE_DIR}/bin)
    set(SDL3_INCLUDE_DIR ${SDL3_SOURCE_DIR}/include)
    set(SDL3_LIB_DIR ${SDL3_SOURCE_DIR}/lib)
    set(SDL3_DIR ${SDL3_SOURCE_DIR}/cmake)
    find_package(SDL3 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3)
else()
    message(FATAL_ERROR "Undefined platform to link SDL3 library.")
endif()
### add sdl3 - end

### add imgui - start
add_subdirectory(${LAKOT_EXTERNAL_PATH}/imgui "${CMAKE_CURRENT_BINARY_DIR}/imgui_build")
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
### add imgui - end

### add assimp - start
if (LAKOT_USE_PLATFORM_WINDOWS)
    set(assimp_DIR ${LAKOT_EXTERNAL_PATH}/assimp/windows/lib/cmake/assimp-5.4)
    find_package(assimp REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)
else()
    message(FATAL_ERROR "Undefined platform to link Assimp library.")
endif()
### add assimp - end

### add stb - start
set(stb_INCLUDE_DIR ${LAKOT_EXTERNAL_PATH}/stb)
target_include_directories(${PROJECT_NAME} PRIVATE ${stb_INCLUDE_DIR})
### add stb - end

### add glm - start
if (LAKOT_USE_PLATFORM_WINDOWS) # v1.0.1
    set(glm_DIR ${LAKOT_EXTERNAL_PATH}/glm/windows/share/glm)
    find_package(glm REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC glm::glm)
elseif(LAKOT_USE_PLATFORM_ANDROID)
    set(glm_DIR ${LAKOT_EXTERNAL_PATH}/glm/android/share/glm)
    find_package(glm REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC glm::glm)
else()
    message(FATAL_ERROR "Undefined platform to link glm library.")
endif()
### add glm - end

if (ANDROID)
    set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libs/${ANDROID_ABI})
elseif (APPLE)
    if (IOS)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            "-framework OpenGLES"
            "-framework Foundation"
            "-framework UIKit"
            "-framework CoreGraphics"
            "-framework QuartzCore")

        set_target_properties(${PROJECT_NAME} PROPERTIES
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.lakot.renderer"
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "SCY"
        )
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE
            "-framework OpenGL"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo")
    endif()
endif()
