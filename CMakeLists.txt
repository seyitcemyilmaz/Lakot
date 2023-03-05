cmake_minimum_required(VERSION 3.2)
project(LAKOT)

set(CMAKE_CXX_STANDARD 20)

set(SRC_DIRECTORY ${CMAKE_SOURCE_DIR}/src)
    set(APPLICATION_DIRECTORY ${SRC_DIRECTORY}/application)
        set(GRAPHICS_DIRECTORY ${APPLICATION_DIRECTORY}/graphics)
            set(CAMERA_DIRECTORY ${GRAPHICS_DIRECTORY}/camera)
            set(RENDER_DIRECTORY ${GRAPHICS_DIRECTORY}/render)
            set(SHADER_DIRECTORY ${GRAPHICS_DIRECTORY}/shader)
        set(RESOURCES_DIRECTORY ${APPLICATION_DIRECTORY}/resources)
    set(CORE_DIRECTORY ${SRC_DIRECTORY}/core)    
set(THIRD_PARTY_DIRECTORY ${CMAKE_SOURCE_DIR}/third_party)

add_subdirectory(${CORE_DIRECTORY})

set(SRC_FILES
    ${SRC_DIRECTORY}/main.cpp

    ${SRC_DIRECTORY}/lakot.h
    ${SRC_DIRECTORY}/lakot.cpp

    ${SRC_DIRECTORY}/application.h
    ${SRC_DIRECTORY}/application.cpp
)

    set(GRAPHICS_FILES
        ${GRAPHICS_DIRECTORY}/gui.h
        ${GRAPHICS_DIRECTORY}/gui.cpp
    )

        set(CAMERA_FILES
            ${CAMERA_DIRECTORY}/camera.h
            ${CAMERA_DIRECTORY}/camera.cpp

            ${CAMERA_DIRECTORY}/cameramanager.h
            ${CAMERA_DIRECTORY}/cameramanager.cpp
        )

        set(RENDER_FILES
            ${RENDER_DIRECTORY}/rendermanager.h
            ${RENDER_DIRECTORY}/rendermanager.cpp
        )

        set(SHADER_FILES
            ${SHADER_DIRECTORY}/shader.h
            ${SHADER_DIRECTORY}/shader.cpp

            ${SHADER_DIRECTORY}/shadermanager.h
            ${SHADER_DIRECTORY}/shadermanager.cpp
        )

    set(RESOURCES_FILES
        ${RESOURCES_DIRECTORY}/IDrawable.h

        ${RESOURCES_DIRECTORY}/mesh.h
        ${RESOURCES_DIRECTORY}/mesh.cpp

        ${RESOURCES_DIRECTORY}/model.h
        ${RESOURCES_DIRECTORY}/model.cpp
    )

include_directories(
    ${SRC_DIRECTORY}
)

add_executable(LAKOT
    ${SRC_FILES}
        ${CONTROLS_FILES}
        ${GRAPHICS_FILES}
            ${RENDER_FILES}
            ${SHADER_FILES}
            ${CAMERA_FILES}
        ${RESOURCES_FILES}
)

target_link_libraries(LAKOT 
    LAKOT_CORE
    ${OPENGL_LIBRARIES}
)
