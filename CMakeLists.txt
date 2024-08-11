cmake_minimum_required(VERSION 3.11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

project(StructureProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LAKOT_USE_GPU "Use GPU" OFF)
if (LAKOT_USE_GPU)
    add_definitions(-DLAKOT_USE_GPU)
endif()

set(LAKOT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/lakot)
    set(UTILITIES_FOLDER ${LAKOT_FOLDER}/utilities)
    set(INTERFACE_FOLDER ${LAKOT_FOLDER}/interface)
    set(ABSTRACT_FOLDER ${LAKOT_FOLDER}/abstract)
    set(CORE_FOLDER ${LAKOT_FOLDER}/core)

set(APPLICATION_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/application)

add_subdirectory(${UTILITIES_FOLDER})
add_subdirectory(${INTERFACE_FOLDER})
add_subdirectory(${ABSTRACT_FOLDER})
add_subdirectory(${CORE_FOLDER})
# add_subdirectory(${APPLICATION_FOLDER})

include_directories(
    ${SRC_DIRECTORY}
)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

add_executable(${CMAKE_PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    LakotUtilities
    LakotInterface
    LakotAbstract
    LakotCore
    # LakotApplication
)

include(GNUInstallDirs)
install(TARGETS ${CMAKE_PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
