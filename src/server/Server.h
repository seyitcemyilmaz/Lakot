#ifndef LAKOT_SERVER_H
#define LAKOT_SERVER_H

#include "network/NetworkManager.h"
#include "database/DatabaseManager.h"

#include "network/controller/AuthController.h"
#include "network/controller/WorldController.h"
#include "network/controller/ChatController.h"
#include "database/repository/RepositoryManager.h"

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

    std::unique_ptr<AuthController> mAuthController;
    std::unique_ptr<WorldController> mWorldController;
    std::unique_ptr<ChatController> mChatController;
};

}

#endif
