cmake_minimum_required(VERSION 3.16)

project(LakotServer)

find_package(libpqxx CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(${PROJECT_NAME}
    main.cpp

    Server.h Server.cpp

    network/NetworkManager.h network/NetworkManager.cpp

    network/controller/BaseController.h
    network/controller/AuthController.h network/controller/AuthController.cpp
    network/controller/WorldController.h network/controller/WorldController.cpp
    network/controller/ChatController.h network/controller/ChatController.cpp

    world/WorldRegistry.h world/WorldRegistry.cpp
    world/MapCatalog.h world/MapCatalog.cpp

    security/PasswordHasher.h security/PasswordHasher.cpp

    database/DatabaseConfig.h
    database/DatabaseManager.h database/DatabaseManager.cpp

    database/repository/BaseRepository.h
    database/repository/AccountRepository.h database/repository/AccountRepository.cpp

    database/repository/RepositoryManager.h database/repository/RepositoryManager.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    LakotConnection
    libpqxx::pqxx
    OpenSSL::Crypto
)
