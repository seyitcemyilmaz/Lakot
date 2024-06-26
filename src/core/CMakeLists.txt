set(GRAPHICSAPI_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/graphicsAPI)
set(HELPER_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/helper)
    set(CAMERA_DIRECTORY ${HELPER_DIRECTORY}/camera)
    set(CONTROLS_DIRECTORY ${HELPER_DIRECTORY}/controls)
    set(SHADER_DIRECTORY ${HELPER_DIRECTORY}/shader)
    set(WINDOW_DIRECTORY ${HELPER_DIRECTORY}/window)
set(INTERFACE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/interface)
set(PLATFORM_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/platform)
set(RESOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource)
    set(LOADERS_DIRECTORY ${RESOURCE_DIRECTORY}/loaders)


set(GRAPHICSAPI_FILES
    ${GRAPHICSAPI_DIRECTORY}/GraphicsAPI.h
    ${GRAPHICSAPI_DIRECTORY}/GraphicsAPI.cpp
)

if (WIN32)
    list(
        APPEND
        GRAPHICSAPI_FILES
        ${GRAPHICSAPI_DIRECTORY}/OpenGLAPI.h
        ${GRAPHICSAPI_DIRECTORY}/OpenGLAPI.cpp
    )
elseif (UNIX)
    list(
        APPEND
        GRAPHICSAPI_FILES
        ${GRAPHICSAPI_DIRECTORY}/OpenGLAPI.h
        ${GRAPHICSAPI_DIRECTORY}/OpenGLAPI.cpp
    )
endif()

set(HELPER_FILES
    ${HELPER_DIRECTORY}/AssimpToGLMHelper.h
    ${HELPER_DIRECTORY}/AssimpToGLMHelper.cpp

    ${HELPER_DIRECTORY}/FileManager.h
    ${HELPER_DIRECTORY}/FileManager.cpp

    ${HELPER_DIRECTORY}/GLMConverter.h
    ${HELPER_DIRECTORY}/GLMConverter.cpp

    ${HELPER_DIRECTORY}/Timer.h
    ${HELPER_DIRECTORY}/Timer.cpp
)

    set(CAMERA_FILES
        ${CAMERA_DIRECTORY}/Camera.h
        ${CAMERA_DIRECTORY}/Camera.cpp

        ${CAMERA_DIRECTORY}/CameraManager.h
        ${CAMERA_DIRECTORY}/CameraManager.cpp
    )

    set(CONTROLS_FILES
        ${CONTROLS_DIRECTORY}/Keyboard.h
        ${CONTROLS_DIRECTORY}/Keyboard.cpp

        ${CONTROLS_DIRECTORY}/Mouse.h
        ${CONTROLS_DIRECTORY}/Mouse.cpp
    )

    set(SHADER_FILES
        ${SHADER_DIRECTORY}/Shader.h
        ${SHADER_DIRECTORY}/Shader.cpp

        ${SHADER_DIRECTORY}/ShaderManager.h
        ${SHADER_DIRECTORY}/ShaderManager.cpp

        ${SHADER_DIRECTORY}/ShaderUniform.h
        ${SHADER_DIRECTORY}/ShaderUniform.cpp
    )

    set(WINDOW_FILES
        ${WINDOW_DIRECTORY}/Window.h
        ${WINDOW_DIRECTORY}/Window.cpp

        ${WINDOW_DIRECTORY}/GLFWWindow.h
        ${WINDOW_DIRECTORY}/GLFWWindow.cpp

        ${WINDOW_DIRECTORY}/WindowFactory.h
        ${WINDOW_DIRECTORY}/WindowFactory.cpp

	${WINDOW_DIRECTORY}/WindowManager.h
	${WINDOW_DIRECTORY}/WindowManager.cpp
)

set(INTERFACE_FILES
    ${INTERFACE_DIRECTORY}/IAnimated.h
    ${INTERFACE_DIRECTORY}/IAnimated.cpp

    ${INTERFACE_DIRECTORY}/IAnimation.h
    ${INTERFACE_DIRECTORY}/IAnimation.cpp

    ${INTERFACE_DIRECTORY}/IBone.h
    ${INTERFACE_DIRECTORY}/IBone.cpp

    ${INTERFACE_DIRECTORY}/IGuiElement.h
    ${INTERFACE_DIRECTORY}/IGuiElement.cpp

    ${INTERFACE_DIRECTORY}/INode.h
    ${INTERFACE_DIRECTORY}/INode.cpp

    ${INTERFACE_DIRECTORY}/ITransformable.h
    ${INTERFACE_DIRECTORY}/ITransformable.cpp
)

set(PLATFORM_FILES
    ${PLATFORM_DIRECTORY}/Platform.h
    ${PLATFORM_DIRECTORY}/Platform.cpp

    ${PLATFORM_DIRECTORY}/WindowsPlatform.h
    ${PLATFORM_DIRECTORY}/WindowsPlatform.cpp

    ${PLATFORM_DIRECTORY}/PlatformFactory.h
    ${PLATFORM_DIRECTORY}/PlatformFactory.cpp
)

set(RESOURCE_FILES
    ${RESOURCE_DIRECTORY}/AnimationResource.h
    ${RESOURCE_DIRECTORY}/AnimationResource.cpp

    ${RESOURCE_DIRECTORY}/BoneResource.h
    ${RESOURCE_DIRECTORY}/BoneResource.cpp

    ${RESOURCE_DIRECTORY}/KeyFrameChannelResource.h
    ${RESOURCE_DIRECTORY}/KeyFrameChannelResource.cpp

    ${RESOURCE_DIRECTORY}/MaterialResource.h
    ${RESOURCE_DIRECTORY}/MaterialResource.cpp

    ${RESOURCE_DIRECTORY}/MeshResource.h
    ${RESOURCE_DIRECTORY}/MeshResource.cpp

    ${RESOURCE_DIRECTORY}/ModelResource.h
    ${RESOURCE_DIRECTORY}/ModelResource.cpp

    ${RESOURCE_DIRECTORY}/NodeResource.h
    ${RESOURCE_DIRECTORY}/NodeResource.cpp

    ${RESOURCE_DIRECTORY}/ResourceManager.h
    ${RESOURCE_DIRECTORY}/ResourceManager.cpp

    ${RESOURCE_DIRECTORY}/TextureResource.h
    ${RESOURCE_DIRECTORY}/TextureResource.cpp
)

    set(LOADERS_FILES
	${LOADERS_DIRECTORY}/AnimationLoader.h
	${LOADERS_DIRECTORY}/AnimationLoader.cpp

        ${LOADERS_DIRECTORY}/BoneWeightLoader.h
        ${LOADERS_DIRECTORY}/BoneWeightLoader.cpp

        ${LOADERS_DIRECTORY}/MaterialLoader.h
        ${LOADERS_DIRECTORY}/MaterialLoader.cpp

        ${LOADERS_DIRECTORY}/MeshLoader.h
        ${LOADERS_DIRECTORY}/MeshLoader.cpp

        ${LOADERS_DIRECTORY}/ModelLoader.h
        ${LOADERS_DIRECTORY}/ModelLoader.cpp

        ${LOADERS_DIRECTORY}/TextureLoader.h
        ${LOADERS_DIRECTORY}/TextureLoader.cpp
    )

add_library(LAKOT_CORE_LIB
    ${GRAPHICSAPI_FILES}
    ${HELPER_FILES}
        ${CAMERA_FILES}
        ${CONTROLS_FILES}
        ${SHADER_FILES}
	${WINDOW_FILES}
    ${INTERFACE_FILES}
    ${PLATFORM_FILES}
    ${RESOURCE_FILES}
        ${LOADERS_FILES}
        interface/IScene.h interface/IScene.cpp
        helper/shader/ShaderProgram.h helper/shader/ShaderProgram.cpp
)

### add glfw - begin ###
if (WIN32)
    find_package(glfw3 CONFIG REQUIRED)
    target_link_libraries(LAKOT_CORE_LIB PRIVATE glfw)
elseif (UNIX)

endif ()
### add glfw - end ###

### add glad - begin ###
if (WIN32)
    find_package(glad CONFIG REQUIRED)
    target_link_libraries(LAKOT_CORE_LIB PRIVATE glad::glad)
elseif (UNIX)

endif()
### add glad - end ###

### add glm - begin ###
if (WIN32)
    find_package(glm CONFIG REQUIRED)
    target_link_libraries(LAKOT_CORE_LIB PRIVATE glm::glm)
elseif (UNIX)

endif()
### add glm - end ###

### add assimp - begin ###
if (WIN32)
    find_package(assimp CONFIG REQUIRED)
    target_link_libraries(LAKOT_CORE_LIB PRIVATE assimp::assimp)
elseif (UNIX)

endif()
### add assimp - end ###

### add stb - begin ###
if (WIN32)
    find_package(Stb REQUIRED)
    target_include_directories(LAKOT_CORE_LIB PRIVATE ${Stb_INCLUDE_DIR})
elseif (UNIX)

endif()
### add stb - end ###

### add spdlog - begin ###
if (WIN32)
    find_package(spdlog CONFIG REQUIRED)
    target_link_libraries(LAKOT_CORE_LIB PRIVATE spdlog::spdlog)
elseif (UNIX)

endif()
### add spdlog - end ###

target_include_directories(LAKOT_CORE_LIB PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
