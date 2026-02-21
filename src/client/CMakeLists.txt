cmake_minimum_required(VERSION 3.16)
project(Client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    main.cpp

    NetworkManager.h NetworkManager.cpp
    controller/AuthController.h controller/AuthController.cpp
)


target_link_libraries(${PROJECT_NAME} PRIVATE LakotConnection)
