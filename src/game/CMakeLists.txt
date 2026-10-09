cmake_minimum_required(VERSION 3.16)

project(LakotGame LANGUAGES CXX)

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
    main.cpp

    Engine.h Engine.cpp

    asset/AssetManager.h asset/AssetManager.cpp

    settings/DisplaySettings.h settings/DisplaySettings.cpp

    gui/GuiLayer.h
    gui/GuiLayer.cpp
    gui/UiTheme.h

    gui/Panel.h
    gui/PanelLoader.h gui/PanelLoader.cpp
    gui/AttachmentPanel.h gui/AttachmentPanel.cpp

    gui/rml/backend/RmlUi_Renderer_GL3.h gui/rml/backend/RmlUi_Renderer_GL3.cpp
    gui/rml/backend/RmlUi_Platform_SDL.h gui/rml/backend/RmlUi_Platform_SDL.cpp
    gui/rml/RmlUiLayer.h gui/rml/RmlUiLayer.cpp
    gui/rml/LoginRmlController.h gui/rml/LoginRmlController.cpp
    gui/rml/CharacterSelectRmlController.h gui/rml/CharacterSelectRmlController.cpp
    gui/rml/ChatRmlController.h gui/rml/ChatRmlController.cpp
    gui/rml/WorldChatRmlController.h gui/rml/WorldChatRmlController.cpp
    gui/rml/SkillBarRmlController.h gui/rml/SkillBarRmlController.cpp
    gui/rml/InventoryRmlController.h gui/rml/InventoryRmlController.cpp
    gui/rml/CharacterSheetRmlController.h gui/rml/CharacterSheetRmlController.cpp
    gui/rml/SystemMenuRmlController.h gui/rml/SystemMenuRmlController.cpp
    gui/rml/ReconnectRmlController.h gui/rml/ReconnectRmlController.cpp
    gui/rml/MinimapRmlController.h gui/rml/MinimapRmlController.cpp
    gui/rml/WorldMapRmlController.h gui/rml/WorldMapRmlController.cpp
    gui/MapImageFactory.h gui/MapImageFactory.cpp
    graphics/model/Model.h graphics/model/Model.cpp
    graphics/model/Texture.h graphics/model/Texture.cpp
    graphics/model/Skeleton.h graphics/model/Skeleton.cpp
    graphics/model/AnimationClip.h graphics/model/AnimationClip.cpp
    graphics/model/SkinnedModel.h graphics/model/SkinnedModel.cpp
    graphics/model/Animator.h graphics/model/Animator.cpp

    network/GameTypes.h
    network/NetworkManager.h network/NetworkManager.cpp
    network/controller/InventoryController.h network/controller/InventoryController.cpp
    network/controller/AuthController.h network/controller/AuthController.cpp
    network/controller/WorldController.h network/controller/WorldController.cpp
    network/controller/ChatController.h network/controller/ChatController.cpp

    graphics/camera/CameraFactory.h graphics/camera/CameraFactory.cpp
    graphics/camera/Camera.h graphics/camera/Camera.cpp
    graphics/camera/FPSCamera.h graphics/camera/FPSCamera.cpp
    graphics/camera/ThirdPersonCamera.h graphics/camera/ThirdPersonCamera.cpp

    graphics/geometry/BoxContainer.h graphics/geometry/BoxContainer.cpp

    graphics/render/Renderable.h graphics/render/Renderable.cpp

    graphics/render/Shader.h graphics/render/Shader.cpp
    graphics/render/ShaderProgram.h graphics/render/ShaderProgram.cpp
    graphics/render/VertexBufferObject.h graphics/render/VertexBufferObject.cpp
    graphics/render/VertexArrayObject.h graphics/render/VertexArrayObject.cpp

    scene/Scene.h
    scene/SceneManager.h scene/SceneManager.cpp
    scene/LoginScene.h scene/LoginScene.cpp
    scene/CharacterSelectScene.h scene/CharacterSelectScene.cpp
    scene/WorldScene.h scene/WorldScene.cpp
    scene/world/WorldViewConstants.h
    scene/world/WorldEntity.h
    scene/world/WorldEntities.h scene/world/WorldEntities.cpp
    scene/world/EntityRenderer.h scene/world/EntityRenderer.cpp
    scene/world/AnimationRoles.h scene/world/AnimationRoles.cpp
    scene/world/EquipmentAttachments.h scene/world/EquipmentAttachments.cpp
    graphics/model/AnimationRetargeter.h graphics/model/AnimationRetargeter.cpp
    graphics/model/MotionLibrary.h graphics/model/MotionLibrary.cpp
    scene/world/DamageNumbers.h scene/world/DamageNumbers.cpp
    gui/rml/VitalsRmlController.h gui/rml/VitalsRmlController.cpp
    item/ItemAppearance.h item/ItemAppearance.cpp
    item/ItemIconRenderer.h item/ItemIconRenderer.cpp
    scene/world/WorldPicker.h scene/world/WorldPicker.cpp
    scene/world/AutoWalk.h scene/world/AutoWalk.cpp
    scene/world/CameraBob.h scene/world/CameraBob.cpp
    scene/world/WhisperWindows.h scene/world/WhisperWindows.cpp
    scene/world/WorldHud.h scene/world/WorldHud.cpp
    graphics/render/ShaderManager.h graphics/render/ShaderManager.cpp
    graphics/geometry/Terrain.h graphics/geometry/Terrain.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE LakotConnection)

### Developer-iteration switches. These used to be commented-out #defines in
### Engine.cpp / LoginRmlController.cpp, so toggling them meant editing source
### (and risked being committed on by accident). As cache options they can be
### flipped per build directory - e.g. Qt Creator's Debug kit - without
### touching the tree.
option(LAKOT_DEV_MODE "Skip the login screen and start directly in WorldScene" OFF)
option(LAKOT_DEV_QUICK_LOGIN "Show the one-click quick-login buttons on the login screen" OFF)
option(LAKOT_DEV_TOOLS "Show in-game tuning panels (F10) that save into the source data folder" ON)

if (LAKOT_DEV_MODE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LAKOT_DEV_MODE)
endif()

if (LAKOT_DEV_QUICK_LOGIN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LAKOT_DEV_QUICK_LOGIN)
endif()

if (LAKOT_DEV_TOOLS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LAKOT_DEV_TOOLS LAKOT_SOURCE_DATA_DIR="${CMAKE_SOURCE_DIR}/data")
endif()

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

### Copy runtime assets (UI documents, stylesheets, fonts) and SDL3.dll next
### to the built executable.
###
### A separate ALL target, NOT a POST_BUILD command on LakotGame. POST_BUILD
### only runs when the executable itself is relinked, so editing only an .rml
### or .rcss and rebuilding left the OLD copy deployed - the game kept loading
### a stale stylesheet while the source on disk was already fixed, which is
### exactly as confusing as it sounds. An ALL custom target runs on every
### build regardless of whether any C++ changed.
add_custom_target(LakotGameAssets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SDL3_BIN_DIR}/SDL3.dll
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/SDL3.dll
    COMMENT "Copying LakotGame assets"
)

add_dependencies(${PROJECT_NAME} LakotGameAssets)

### add imgui - start
add_subdirectory(${LAKOT_EXTERNAL_PATH}/imgui "${CMAKE_CURRENT_BINARY_DIR}/imgui_build")
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
### add imgui - end

### add rmlui - start
### Player-facing UI (login, chat, later inventory/skills) - via vcpkg
### (VCPKG_ROOT, same as Protobuf/libpqxx), not LAKOT_EXTERNAL: the port
### already exists on vcpkg and is less total work than vendoring+building
### RmlUi from source on every build. ImGui (gui/GuiLayer, gui/Panel*)
### remains untouched for developer/debug tooling only.
find_package(RmlUi CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE RmlUi::RmlUi)

### Backends/RmlUi_Renderer_GL3.cpp (vendored under gui/rml/backend/, see
### that folder's note) supports plugging in an already-loaded GL loader
### instead of bundling its own - point it at the glad already loaded once
### in main.cpp so there's only one GL loader in the process.
target_compile_definitions(${PROJECT_NAME} PRIVATE
    RMLUI_GL3_CUSTOM_LOADER=<glad/glad.h>
    RMLUI_SDL_VERSION_MAJOR=3
)
### add rmlui - end

### add assimp - start
if (LAKOT_USE_PLATFORM_WINDOWS)
    set(assimp_DIR ${LAKOT_EXTERNAL_PATH}/assimp/windows/lib/cmake/assimp-5.4)
    find_package(assimp REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)

    # The DLL next to the executable, like SDL3.dll above - without it the
    # game does not start at all now that it actually loads models.
    add_custom_command(TARGET LakotGameAssets POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:assimp::assimp>
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
    )
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
