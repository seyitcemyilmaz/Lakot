#ifndef LAKOT_SERVER_H
#define LAKOT_SERVER_H

#include "network/NetworkManager.h"
#include "database/DatabaseManager.h"

#include "network/controller/AuthController.h"
#include "network/controller/CharacterController.h"
#include "network/controller/WorldController.h"
#include "network/controller/ChatController.h"
#include "network/controller/InventoryController.h"
#include "database/repository/RepositoryManager.h"
#include "game/ItemCatalog.h"
#include "MonsterCatalog.h"

namespace lakot
{

class Server
{
public:
    virtual ~Server();
    explicit Server();

    bool initialize();

    void run();

private:
    DatabaseManager mDatabaseManager;
    NetworkManager mNetworkManager;

    std::unique_ptr<RepositoryManager> mRepositoryManager;

    // Owned here rather than by a controller because it outlives and is
    // shared by all of them (and, later, by every zone thread and Lua state).
    // Filled during initialize() and immutable afterwards - see ItemCatalog.
    ItemCatalog mItemCatalog;

    // Maps and kingdoms from data/. Loaded before any controller starts;
    // read-only afterwards.
    MapCatalog mMapCatalog;
    MonsterCatalog mMonsterCatalog;

    std::unique_ptr<WorldController> mWorldController;
    std::unique_ptr<InventoryController> mInventoryController;
    std::unique_ptr<ChatController> mChatController;
    std::unique_ptr<CharacterController> mCharacterController;
    std::unique_ptr<AuthController> mAuthController;
};

}

#endif
