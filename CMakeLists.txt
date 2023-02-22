cmake_minimum_required(VERSION 3.2)
project(LAKOT)

set(CMAKE_CXX_STANDARD 20)

set(SRC_DIRECTORY ${CMAKE_SOURCE_DIR}/src)
    set(APPLICATION_DIRECTORY ${SRC_DIRECTORY}/application)
        set(GRAPHICS_DIRECTORY ${APPLICATION_DIRECTORY}/graphics)
            set(RENDER_DIRECTORY ${GRAPHICS_DIRECTORY}/render)
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
        ${GRAPHICS_DIRECTORY}/shader.h
        ${GRAPHICS_DIRECTORY}/shader.cpp
        ${GRAPHICS_DIRECTORY}/shadermanager.h
        ${GRAPHICS_DIRECTORY}/shadermanager.cpp
    )

        set(RENDER_FILES
            ${RENDER_DIRECTORY}/renderer.h
            ${RENDER_DIRECTORY}/renderer.cpp
        )
#[[
    set(HELPER_FILES
        ${HELPER_DIRECTORY}/constants.h

        ${HELPER_DIRECTORY}/contextprovider.h
        ${HELPER_DIRECTORY}/contextprovider.cpp
        ${HELPER_DIRECTORY}/platform.h
        ${HELPER_DIRECTORY}/platform.cpp
        ${HELPER_DIRECTORY}/windowmanager.h
        ${HELPER_DIRECTORY}/windowmanager.cpp
    )
]]

include_directories(
    ${SRC_DIRECTORY}
)

add_executable(LAKOT
    ${SRC_FILES}
        ${GRAPHICS_FILES}
            ${RENDER_FILES}
)

target_link_libraries(LAKOT 
    LAKOT_CORE
    ${OPENGL_LIBRARIES}
)
