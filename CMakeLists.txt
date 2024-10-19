cmake_minimum_required(VERSION 3.22.1)

project(Lakot LANGUAGES CXX)

include(${CMAKE_SOURCE_DIR}/lakot/cmake/LakotSettings.cmake)

set(LAKOT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/lakot)
    set(UTILITIES_FOLDER ${LAKOT_FOLDER}/utilities)
    set(ABSTRACT_FOLDER ${LAKOT_FOLDER}/abstract)
    set(CORE_FOLDER ${LAKOT_FOLDER}/core)

set(APPLICATION_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/application)

add_subdirectory(${UTILITIES_FOLDER})
add_subdirectory(${ABSTRACT_FOLDER})
add_subdirectory(${CORE_FOLDER})
# add_subdirectory(${APPLICATION_FOLDER})

set(LAKOT_TOP_LEVEL_FILES
    main.cpp
)

if(LAKOT_USE_PLATFORM_WINDOWS OR LAKOT_USE_PLATFORM_LINUX)
    add_executable(${CMAKE_PROJECT_NAME}
        ${LAKOT_TOP_LEVEL_FILES}
    )
endif()

if(LAKOT_USE_PLATFORM_ANDROID)
    add_library(${CMAKE_PROJECT_NAME} SHARED
        ${LAKOT_TOP_LEVEL_FILES}
    )
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    LakotUtilities
    LakotAbstract
    LakotCore
    # LakotApplication
)

if(LAKOT_USE_PLATFORM_WINDOWS)
    include(GNUInstallDirs)
    install(TARGETS ${CMAKE_PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
