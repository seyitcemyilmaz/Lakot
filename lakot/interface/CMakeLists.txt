cmake_minimum_required(VERSION 3.11)

project(LakotInterface LANGUAGES CXX)

add_library(${PROJECT_NAME} INTERFACE
    engine/IEngine.h
    engine/ILogger.h
    engine/IObject.h

    graphics/api/IGraphicsAPI.h
    graphics/window/IWindow.h
    graphics/camera/ICamera.h

    graphics/render/IRenderer.h
    graphics/render/IRenderable.h

    layer/ILayer.h

    resource/asset/IAsset.h
    resource/asset/IModelAsset.h
    resource/asset/INodeAsset.h
    resource/asset/IMeshAsset.h

    resource/entity/IEntity.h

    resource/loader/ILoader.h
)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../..)

### add glm - begin
if (LAKOT_USE_PLATFORM_WINDOWS) # v1.0.1
    set(glm_DIR ${LAKOT_EXTERNAL_PATH}/glm/windows/share/glm)
    find_package(glm REQUIRED)
    target_link_libraries(${PROJECT_NAME} INTERFACE glm::glm)
elseif(LAKOT_USE_PLATFORM_ANDROID)
    set(glm_DIR ${LAKOT_EXTERNAL_PATH}/glm/android/share/glm)
    find_package(glm REQUIRED)
    target_link_libraries(${PROJECT_NAME} INTERFACE glm::glm)
else()
    message(FATAL_ERROR "Undefined platform to link assimp library.")
endif()
### add glm - end

### add spdlog - begin ###
if (LAKOT_USE_PLATFORM_WINDOWS) # v1.14.1
    if (LAKOT_USE_BUILD_TYPE_RELEASE)
        set(spdlog_DIR ${LAKOT_EXTERNAL_PATH}/spdlog/windows/release/lib/cmake/spdlog)
        find_package(spdlog REQUIRED)
        target_link_libraries(${PROJECT_NAME} INTERFACE spdlog::spdlog)
    elseif(LAKOT_USE_BUILD_TYPE_DEBUG)
        set(spdlog_DIR ${LAKOT_EXTERNAL_PATH}/spdlog/windows/debug/lib/cmake/spdlog)
        find_package(spdlog REQUIRED)
        target_link_libraries(${PROJECT_NAME} INTERFACE spdlog::spdlog)
    endif()
elseif(LAKOT_USE_PLATFORM_ANDROID)
    set(spdlog_DIR ${LAKOT_EXTERNAL_PATH}/spdlog/android/lib/cmake/spdlog)
    find_package(spdlog REQUIRED)
    target_link_libraries(${PROJECT_NAME} INTERFACE spdlog::spdlog)
else()
    message(FATAL_ERROR "Undefined platform to link assimp library.")
endif()
### add spdlog - end ###
