cmake_minimum_required(VERSION 3.16)

project(LakotServer)

find_package(libpqxx CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

### Lua + sol2 - gameplay scripting (see src/server/script/ScriptEngine.h).
###
### Lua is PINNED to 5.4.x. vcpkg's current port is 5.5.0, which sol2 3.5.0
### rejects outright ("unsupported Lua version" - it supports 5.1 through
### 5.4). The 5.4.8 port is installed from a vcpkg overlay port.
find_package(Lua REQUIRED)

if (LUA_VERSION_STRING AND LUA_VERSION_STRING VERSION_GREATER_EQUAL "5.5")
    message(FATAL_ERROR
        "Lua ${LUA_VERSION_STRING} found, but sol2 only supports up to 5.4. "
        "Reinstall the pinned port: vcpkg install lua:x64-windows --overlay-ports=C:/vcpkg/overlay-ports")
endif()

find_path(SOL2_INCLUDE_DIRS "sol/sol.hpp" REQUIRED)

add_executable(${PROJECT_NAME}
    main.cpp

    Server.h Server.cpp

    network/NetworkManager.h network/NetworkManager.cpp
    network/SessionResumeManager.h network/SessionResumeManager.cpp

    network/controller/BaseController.h
    network/controller/AuthController.h network/controller/AuthController.cpp
    network/controller/CharacterController.h network/controller/CharacterController.cpp
    network/controller/WorldController.h network/controller/WorldController.cpp
    network/controller/ChatController.h network/controller/ChatController.cpp
    network/controller/InventoryController.h network/controller/InventoryController.cpp

    game/ItemTemplate.h
    game/ItemCatalog.h game/ItemCatalog.cpp
    game/CharacterStats.h
    game/Inventory.h game/Inventory.cpp
    game/Character.h game/Character.cpp

    script/ScriptEngine.h script/ScriptEngine.cpp

    world/Zone.h world/Zone.cpp
    world/WorldRegistry.h world/WorldRegistry.cpp
    world/CombatSystem.h world/CombatSystem.cpp
    world/MonsterBrain.h world/MonsterBrain.cpp
    world/LootSystem.h world/LootSystem.cpp
    world/Monster.h world/Npc.h

    security/PasswordHasher.h security/PasswordHasher.cpp
    security/LoginAttemptLimiter.h security/LoginAttemptLimiter.cpp

    database/DatabaseConfig.h
    database/DatabaseManager.h database/DatabaseManager.cpp

    network/RequestLimits.h

    database/repository/BaseRepository.h
    database/repository/AccountRepository.h database/repository/AccountRepository.cpp
    database/repository/CharacterRepository.h database/repository/CharacterRepository.cpp
    database/repository/ItemRepository.h database/repository/ItemRepository.cpp

    database/repository/RepositoryManager.h database/repository/RepositoryManager.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    LakotConnection
    libpqxx::pqxx
    OpenSSL::Crypto
    ${LUA_LIBRARIES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${LUA_INCLUDE_DIR}
    ${SOL2_INCLUDE_DIRS}
)

### Content scripts next to the executable.
###
### An ALL target rather than a POST_BUILD command, for the same reason as
### LakotGame's assets: POST_BUILD only fires when the executable relinks, so
### editing only a .lua and rebuilding would leave the old script deployed -
### the one thing this whole scripting layer exists to make quick.
add_custom_target(LakotServerScripts ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scripts
    COMMENT "Copying LakotServer scripts"
)

add_dependencies(${PROJECT_NAME} LakotServerScripts)
