cmake_minimum_required(VERSION 3.16)

project(LakotServer)

find_package(libpqxx CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
    main.cpp

    Server.h Server.cpp

    network/NetworkManager.h network/NetworkManager.cpp

    network/controller/BaseController.h
    network/controller/AuthController.h network/controller/AuthController.cpp

    database/DatabaseConfig.h
    database/DatabaseManager.h database/DatabaseManager.cpp

    database/repository/BaseRepository.h
    database/repository/AccountRepository.h database/repository/AccountRepository.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    LakotConnection
    libpqxx::pqxx
)
