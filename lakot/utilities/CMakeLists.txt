cmake_minimum_required(VERSION 3.11)

project(LakotUtilities LANGUAGES CXX)

add_library(${PROJECT_NAME}
    FileManager.h FileManager.cpp
    Timer.h Timer.cpp
    Type.h
    Object.h Object.cpp
    BoundingBox.h BoundingBox.cpp
    DataContainer.hpp
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

### add spdlog - begin ###
if (LAKOT_USE_PLATFORM_WINDOWS) # v1.14.1
    if (LAKOT_USE_BUILD_TYPE_RELEASE)
        set(spdlog_DIR ${LAKOT_EXTERNAL_PATH}/spdlog/windows/release/lib/cmake/spdlog)
        find_package(spdlog REQUIRED)
        target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog)
    elseif(LAKOT_USE_BUILD_TYPE_DEBUG)
        set(spdlog_DIR ${LAKOT_EXTERNAL_PATH}/spdlog/windows/debug/lib/cmake/spdlog)
        find_package(spdlog REQUIRED)
        target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog)
    endif()
elseif(LAKOT_USE_PLATFORM_ANDROID)
    set(spdlog_DIR ${LAKOT_EXTERNAL_PATH}/spdlog/android/lib/cmake/spdlog)
    find_package(spdlog REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog)
else()
    message(FATAL_ERROR "Undefined platform to link assimp library.")
endif()

### add spdlog - end ###

### add glm - begin ###
if (LAKOT_USE_PLATFORM_WINDOWS) # v1.0.1
    set(glm_DIR ${LAKOT_EXTERNAL_PATH}/glm/windows/share/glm)
    find_package(glm REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC glm::glm)
elseif(LAKOT_USE_PLATFORM_ANDROID)
    set(glm_DIR ${LAKOT_EXTERNAL_PATH}/glm/android/share/glm)
    find_package(glm REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC glm::glm)
else()
    message(FATAL_ERROR "Undefined platform to link assimp library.")
endif()
### add glm - end ###
