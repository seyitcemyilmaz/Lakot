cmake_minimum_required(VERSION 3.16)

project(LakotUtils LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
    utils_global.h

    DataContainer.hpp

    ThreadSafeQueue.hpp

    Task.hpp

    AsynchronousTaskExecutor.h AsynchronousTaskExecutor.cpp
    SynchronousTaskExecutor.h SynchronousTaskExecutor.cpp

    Timer.h Timer.cpp

    BoundingBox.h BoundingBox.cpp

    FileManager.h FileManager.cpp

    Image.h
    Variable.h
    Type.h
    GarbageCollector.h GarbageCollector.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(${PROJECT_NAME} PUBLIC LAKOT_UTILS_LIBRARY)

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
