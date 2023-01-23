cmake_minimum_required(VERSION 3.1)
project(LAKOT)

set(CMAKE_CXX_STANDARD 17)

set(SRC_DIRECTORY ${CMAKE_SOURCE_DIR}/src)
    set(CORE_DIRECTORY ${SRC_DIRECTORY}/core)
    set(GRAPHICS_DIRECTORY ${SRC_DIRECTORY}/graphics)
    set(HELPER_DIRECTORY ${SRC_DIRECTORY}/helper)
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
        ${GRAPHICS_DIRECTORY}/renderer.h
        ${GRAPHICS_DIRECTORY}/renderer.cpp
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
        ${HELPER_FILES}
        ${GRAPHICS_FILES}
)

target_link_libraries(LAKOT 
    LAKOT_CORE
    ${OPENGL_LIBRARIES}
)
