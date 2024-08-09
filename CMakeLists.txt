cmake_minimum_required(VERSION 3.5)

set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_PREFIX_PATH $ENV{VCPKG_ROOT}/installed/x64-windows/share)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

cmake_host_system_information(RESULT CMake_MSVC_PARALLEL QUERY NUMBER_OF_PHYSICAL_CORES)

if(CMake_MSVC_PARALLEL)
  if(CMake_MSVC_PARALLEL GREATER 0)
    message(${CMake_MSVC_PARALLEL})
    add_definitions(/MP${CMake_MSVC_PARALLEL})
  else()
    add_definitions(/MP)
  endif()
endif()

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

add_executable(StructureProject
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_link_libraries(StructureProject PRIVATE
    LakotUtilities
    LakotInterface
    LakotAbstract
    LakotCore
    # LakotApplication
)

### add spdlog - begin ###
if (WIN32)
    find_package(spdlog CONFIG REQUIRED)
    target_link_libraries(StructureProject PRIVATE spdlog::spdlog)
elseif (UNIX)

endif()
### add spdlog - end ###

include(GNUInstallDirs)
install(TARGETS StructureProject
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
