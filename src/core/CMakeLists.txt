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

add_library(LAKOT_CORE
    platform.h
    platform.cpp
    ${GRAPHICSAPI_FILES}
    ${HELPER_FILES}
        ${CONTROLS_FILES}
    ${RESOURCE_FILES}
)

### add glfw - begin ###
if (WIN32)
    find_package(OpenGL REQUIRED)

    add_library(GLFW_LIBRARY SHARED IMPORTED)

    set_target_properties(GLFW_LIBRARY PROPERTIES
      IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw3.dll
      IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/libglfw3dll.a
    )

    target_include_directories(LAKOT_CORE PUBLIC ${THIRD_PARTY_DIRECTORY}/glfw/include)
    target_link_libraries(LAKOT_CORE GLFW_LIBRARY)
elseif (UNIX)
    find_package(OpenGL REQUIRED)

    include_directories(${OPENGL_INCLUDE_DIRS})

    find_package(glfw3 REQUIRED)
endif ()
### add glfw - end ###

### add glad - begin ###
if (WIN32)
    add_subdirectory(${THIRD_PARTY_DIRECTORY}/glad GLAD_FILES)
    target_link_libraries(LAKOT_CORE glad)
elseif (UNIX)
    add_subdirectory(${THIRD_PARTY_DIRECTORY}/glad)
#elseif(ANDROID)
    #add_subdirectory(${THIRD_PARTY_DIRECTORY}/glad)
endif()
### add glad - end ###

### add glm - begin ###
if (WIN32)
    add_subdirectory(${THIRD_PARTY_DIRECTORY}/glm GLM_FILES)
    include_directories(${THIRD_PARTY_DIRECTORY}/glm)
    target_link_libraries(LAKOT_CORE glm)
elseif (UNIX)
    add_subdirectory(${THIRD_PARTY_DIRECTORY}/glm)
    include_directories(${THIRD_PARTY_DIRECTORY}/glm)
endif()
### add glm - end ###

### add assimp - begin ###
if (WIN32)
    add_library(ASSIMP_LIBRARY SHARED IMPORTED)

    set_target_properties(ASSIMP_LIBRARY PROPERTIES
      IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/libassimp-5d.dll
      IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/libassimpd.dll.a
    )

    target_include_directories(LAKOT_CORE PUBLIC ${THIRD_PARTY_DIRECTORY}/assimp/include)
    target_link_libraries(LAKOT_CORE ASSIMP_LIBRARY)
elseif (UNIX)

endif()
### add assimp - end ###

### add stb - begin ###
if (WIN32)
    add_library(STB_LIBRARY INTERFACE ${THIRD_PARTY_DIRECTORY}/stb/stb_image.h)

    target_include_directories(STB_LIBRARY INTERFACE ${THIRD_PARTY_DIRECTORY}/stb)
    target_link_libraries(LAKOT_CORE STB_LIBRARY)
elseif (UNIX)

endif()
### add stb - end ###

target_include_directories(LAKOT_CORE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_directories(LAKOT_CORE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)
