set(GRAPHICSAPI_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/graphicsAPI)
set(HELPER_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/helper)
    set(CONTROLS_DIRECTORY ${HELPER_DIRECTORY}/controls)
set(RESOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource)


set(GRAPHICSAPI_FILES
    ${GRAPHICSAPI_DIRECTORY}/graphicsapi.h
    ${GRAPHICSAPI_DIRECTORY}/graphicsapi.cpp
)

if (WIN32)
    list(
        APPEND
        GRAPHICSAPI_FILES
        ${GRAPHICSAPI_DIRECTORY}/openglapi.h
        ${GRAPHICSAPI_DIRECTORY}/openglapi.cpp
    )
elseif (UNIX)
    list(
        APPEND
        GRAPHICSAPI_FILES
        ${GRAPHICSAPI_DIRECTORY}/openglapi.h
        ${GRAPHICSAPI_DIRECTORY}/openglapi.cpp
    )
endif()

set(HELPER_FILES
    ${HELPER_DIRECTORY}/FileManager.h
    ${HELPER_DIRECTORY}/FileManager.cpp

    ${HELPER_DIRECTORY}/timer.h
    ${HELPER_DIRECTORY}/timer.cpp

    ${HELPER_DIRECTORY}/windowmanager.h
    ${HELPER_DIRECTORY}/windowmanager.cpp
)

set(RESOURCE_FILES
    ${RESOURCE_DIRECTORY}/boneresource.h
    ${RESOURCE_DIRECTORY}/boneresource.cpp

    ${RESOURCE_DIRECTORY}/meshresource.h
    ${RESOURCE_DIRECTORY}/meshresource.cpp

    ${RESOURCE_DIRECTORY}/modelloader.h
    ${RESOURCE_DIRECTORY}/modelloader.cpp

    ${RESOURCE_DIRECTORY}/modelresource.h
    ${RESOURCE_DIRECTORY}/modelresource.cpp

    ${RESOURCE_DIRECTORY}/noderesource.h
    ${RESOURCE_DIRECTORY}/noderesource.cpp

    ${RESOURCE_DIRECTORY}/resourcemanager.h
    ${RESOURCE_DIRECTORY}/resourcemanager.cpp

    ${RESOURCE_DIRECTORY}/textureloader.h
    ${RESOURCE_DIRECTORY}/textureloader.cpp

    ${RESOURCE_DIRECTORY}/textureresource.h
    ${RESOURCE_DIRECTORY}/textureresource.cpp
)

    set(CONTROLS_FILES
        ${CONTROLS_DIRECTORY}/keyboard.h
        ${CONTROLS_DIRECTORY}/keyboard.cpp

        ${CONTROLS_DIRECTORY}/mouse.h
        ${CONTROLS_DIRECTORY}/mouse.cpp
    )

add_library(LAKOT_CORE_LIB
    platform.h
    platform.cpp
    ${GRAPHICSAPI_FILES}
    ${HELPER_FILES}
        ${CONTROLS_FILES}
    ${RESOURCE_FILES}
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

target_include_directories(LAKOT_CORE_LIB PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
