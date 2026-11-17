cmake_minimum_required(VERSION 3.16)

project(LakotUtils LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
    utils_global.h

    DataContainer.hpp

    ThreadSafeQueue.hpp

    Task.hpp

    AsynchronousTaskExecutor.h AsynchronousTaskExecutor.cpp
    SynchronousTaskExecutor.h SynchronousTaskExecutor.cpp

    MapData.h MapData.cpp
    MapCatalog.h MapCatalog.cpp
    MonsterCatalog.h MonsterCatalog.cpp
    ClassCatalog.h ClassCatalog.cpp
    SkillCatalog.h SkillCatalog.cpp
    Pathfinder.h Pathfinder.cpp
    MovementRules.h
    CombatRules.h
    ItemRules.h

    FileManager.h FileManager.cpp

    Type.h
    GarbageCollector.h GarbageCollector.cpp

    CrashTrigger.h CrashTrigger.cpp
    diagnostics/CrashClassifier.h diagnostics/CrashClassifier.cpp
    diagnostics/CrashHandler.h
    diagnostics/ErrorReport.h diagnostics/ErrorReport.cpp
    diagnostics/LogRing.h diagnostics/LogRing.cpp
    diagnostics/LogTee.h diagnostics/LogTee.cpp
    diagnostics/ThreadName.h diagnostics/ThreadName.cpp
)

### Diagnostics: crash handler, reports, build commit - start
if (WIN32)
    target_sources(${PROJECT_NAME} PRIVATE
        diagnostics/CrashHandler.cpp
        diagnostics/CrashReportWriter.h diagnostics/CrashReportWriter.cpp
        diagnostics/ErrorReportWorker.h diagnostics/ErrorReportWorker.cpp
        diagnostics/ExceptionTrace.h diagnostics/ExceptionTrace.cpp
        diagnostics/StackSymbolizer.h diagnostics/StackSymbolizer.cpp
    )
    target_link_libraries(${PROJECT_NAME} PUBLIC dbghelp)
else()
    target_sources(${PROJECT_NAME} PRIVATE diagnostics/CrashHandlerPosix.cpp)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE LAKOT_SOURCE_ROOT="${CMAKE_SOURCE_DIR}")

set(LAKOT_BUILD_INFO_HEADER ${CMAKE_BINARY_DIR}/generated/LakotBuildInfo.h)
set(LAKOT_BUILD_INFO_ARGUMENTS
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DOUTPUT_FILE=${LAKOT_BUILD_INFO_HEADER}
    -P ${CMAKE_SOURCE_DIR}/cmake/LakotBuildInfo.cmake)

execute_process(COMMAND ${CMAKE_COMMAND} ${LAKOT_BUILD_INFO_ARGUMENTS})

add_custom_target(LakotBuildInfo
    COMMAND ${CMAKE_COMMAND} ${LAKOT_BUILD_INFO_ARGUMENTS}
    BYPRODUCTS ${LAKOT_BUILD_INFO_HEADER}
    VERBATIM)
add_dependencies(${PROJECT_NAME} LakotBuildInfo)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)
### Diagnostics - end

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(${PROJECT_NAME} PUBLIC LAKOT_UTILS_LIBRARY)

### Map data: PNG layers through stb_image, map.json through RapidJSON.
find_package(RapidJSON CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE RapidJSON)
target_include_directories(${PROJECT_NAME} PRIVATE ${LAKOT_EXTERNAL_PATH}/stb)

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
