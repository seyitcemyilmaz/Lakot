#ifndef LAKOT_SERVER_H
#define LAKOT_SERVER_H

#include "network/NetworkManager.h"
#include "database/DatabaseManager.h"

#include "network/controller/AuthController.h"
#include "database/repository/AccountRepository.h"

namespace lakot
{

class Server
{
public:
    virtual ~Server();
    Server();

    bool initialize();

    void run();

private:
    DatabaseManager mDatabaseManager;
    NetworkManager mNetworkManager;

    std::unique_ptr<AuthController> mAuthController;

    std::unique_ptr<AccountRepository> mAccountRepository;
};

}

#endif
