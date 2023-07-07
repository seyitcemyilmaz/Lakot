cmake_minimum_required(VERSION 3.5)

set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
#set(CMAKE_PREFIX_PATH $ENV{VCPKG_ROOT}/installed/x64-windows/share)

cmake_host_system_information(RESULT CMake_MSVC_PARALLEL QUERY NUMBER_OF_PHYSICAL_CORES)

if(CMake_MSVC_PARALLEL)
  if(CMake_MSVC_PARALLEL GREATER 0)
      message(${CMake_MSVC_PARALLEL})
    add_definitions(/MP${CMake_MSVC_PARALLEL})
  else()
    add_definitions(/MP)
  endif()
endif()

project(LAKOT)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRC_DIRECTORY ${CMAKE_SOURCE_DIR}/src)
    set(APPLICATION_DIRECTORY ${SRC_DIRECTORY}/application)
        set(GRAPHICS_DIRECTORY ${APPLICATION_DIRECTORY}/graphics)
            set(RENDER_DIRECTORY ${GRAPHICS_DIRECTORY}/render)
            set(SHADER_DIRECTORY ${GRAPHICS_DIRECTORY}/shader)
        set(RESOURCES_DIRECTORY ${APPLICATION_DIRECTORY}/resources)
    set(CORE_DIRECTORY ${SRC_DIRECTORY}/core)    
set(THIRD_PARTY_DIRECTORY ${CMAKE_SOURCE_DIR}/third_party)

add_subdirectory(${CORE_DIRECTORY})

set(SRC_FILES
    ${SRC_DIRECTORY}/main.cpp

    ${SRC_DIRECTORY}/Lakot.h
    ${SRC_DIRECTORY}/Lakot.cpp

    ${SRC_DIRECTORY}/Application.h
    ${SRC_DIRECTORY}/Application.cpp
)

    set(GRAPHICS_FILES
        ${GRAPHICS_DIRECTORY}/GUI.h
        ${GRAPHICS_DIRECTORY}/GUI.cpp
    )

        set(RENDER_FILES
            ${RENDER_DIRECTORY}/RenderManager.h
            ${RENDER_DIRECTORY}/RenderManager.cpp
        )

        set(SHADER_FILES
            ${SHADER_DIRECTORY}/Shader.h
            ${SHADER_DIRECTORY}/Shader.cpp

            ${SHADER_DIRECTORY}/ShaderManager.h
            ${SHADER_DIRECTORY}/ShaderManager.cpp
        )

    set(RESOURCES_FILES
        ${RESOURCES_DIRECTORY}/IDrawable.h

        ${RESOURCES_DIRECTORY}/Mesh.h
        ${RESOURCES_DIRECTORY}/Mesh.cpp

        ${RESOURCES_DIRECTORY}/Model.h
        ${RESOURCES_DIRECTORY}/Model.cpp
    )

include_directories(
    ${SRC_DIRECTORY}
    $ENV{VCPKG_ROOT}/installed/x64-windows/include
)

if(CMAKE_BUILD_TYPE EQUAL "DEBUG")
    link_directories(
        $ENV{VCPKG_ROOT}/installed/x64-windows/debug/lib
    )
else()
    link_directories(
        $ENV{VCPKG_ROOT}/installed/x64-windows/lib
    )
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

add_executable(${CMAKE_PROJECT_NAME}
    ${SRC_FILES}
        ${CONTROLS_FILES}
        ${GRAPHICS_FILES}
            ${RENDER_FILES}
            ${SHADER_FILES}
        ${RESOURCES_FILES}
)

target_link_libraries(${CMAKE_PROJECT_NAME} 
    LAKOT_CORE_LIB
    ${OPENGL_LIBRARIES}
)
