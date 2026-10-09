set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(LAKOT_USE_PLATFORM_WINDOWS "Windows Platform" OFF)
option(LAKOT_USE_PLATFORM_LINUX "Linux Platform" OFF)
option(LAKOT_USE_PLATFORM_ANDROID "Android Platform" OFF)

option(LAKOT_USE_RENDERER_OPENGL "Use OpenGL" OFF)
set(LAKOT_USE_RENDERER_OPENGL_TYPE "" CACHE STRING "OpenGL Renderer Type (Core or ES)")
set_property(CACHE LAKOT_USE_RENDERER_OPENGL_TYPE PROPERTY STRINGS "Core" "ES")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(-DLAKOT_PLATFORM_WINDOWS)

    # Target Windows API baseline. Without this, every Boost.Asio translation
    # unit warned and silently assumed 0x0601 (Windows 7) - which also caps
    # which Winsock APIs Asio is allowed to use. 0x0A00 is Windows 10; the
    # game already requires OpenGL 4.6 core and SDL3, so nothing older was
    # realistically a target anyway. Lower this line if that ever changes.
    add_definitions(-D_WIN32_WINNT=0x0A00)

    # windows.h defines min/max as macros, which breaks any use of std::min /
    # std::max (WorldScene and the AoI code both rely on them) the moment it
    # gets pulled in - by Boost.Asio, SDL, or a future dependency. WIN32_LEAN_AND_MEAN
    # additionally keeps the rarely-needed half of windows.h out of every
    # translation unit that transitively includes it.
    add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)

    set(LAKOT_USE_PLATFORM_WINDOWS ON)
    set(LAKOT_USE_RENDERER_OPENGL ON)
    set(LAKOT_USE_RENDERER_OPENGL_TYPE "Core")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-DLAKOT_PLATFORM_LINUX)
    set(LAKOT_USE_PLATFORM_LINUX ON)
    set(LAKOT_USE_RENDERER_OPENGL ON)
    set(LAKOT_USE_RENDERER_OPENGL_TYPE "Core")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_definitions(-DLAKOT_PLATFORM_ANDROID)
    set(LAKOT_USE_PLATFORM_ANDROID ON)
    set(LAKOT_USE_RENDERER_OPENGL ON)
    set(LAKOT_USE_RENDERER_OPENGL_TYPE "ES")
endif()

if(LAKOT_USE_RENDERER_OPENGL)
    add_definitions(-DLAKOT_RENDERER_OPENGL)

    if (LAKOT_USE_RENDERER_OPENGL_TYPE STREQUAL "Core")
        add_definitions(-DLAKOT_RENDERER_OPENGL_TYPE_CORE)
    elseif (LAKOT_USE_RENDERER_OPENGL_TYPE STREQUAL "ES")
        add_definitions(-DLAKOT_RENDERER_OPENGL_TYPE_ES)
    else()
        message(FATAL_ERROR "OpenGL Type is not found.")
    endif()
else()
    message(FATAL_ERROR "Renderer is not found.")
endif()

option(LAKOT_USE_GPU "Use GPU" OFF)

if(LAKOT_USE_PLATFORM_WINDOWS OR LAKOT_USE_PLATFORM_LINUX)
    if(LAKOT_USE_GPU)
        add_definitions(-DLAKOT_USE_GPU)
    endif()
endif()

set(LAKOT_EXTERNAL_PATH "$ENV{LAKOT_EXTERNAL}")

option(LAKOT_USE_BUILD_TYPE_DEBUG "Lakot Debug Build" OFF)
option(LAKOT_USE_BUILD_TYPE_RELEASE "Lakot Release Build" OFF)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(LAKOT_USE_BUILD_TYPE_RELEASE ON)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LAKOT_USE_BUILD_TYPE_DEBUG ON)
else()
    message(FATAL_ERROR "Undefined build type.")
endif()

# TODO: vcpkg dependency should be removed.
set(VCPKG_PATH "$ENV{VCPKG_ROOT}")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
set(CMAKE_PREFIX_PATH $ENV{VCPKG_ROOT}/installed/x64-windows/share)
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_PATH}/installed/x64-windows")
