#include "Server.h"

using namespace lakot;

Server::~Server()
{
    mNetworkManager.stop();
    mDatabaseManager.stop();
}

Server::Server()
    : mDatabaseManager()
    , mNetworkManager()
{
    mAuthController = std::make_unique<AuthController>(mNetworkManager);

    mAccountRepository = std::make_unique<AccountRepository>(mDatabaseManager);
}

bool Server::initialize()
{
    std::cout << "--- Lakot Game Server Baslatiliyor ---" << std::endl;

    DatabaseConfig tDatabaseConfig;

    mDatabaseManager.initialize(tDatabaseConfig);

    mAccountRepository->initializeTable();

    mAuthController->initialize();

    mNetworkManager.start();

    return true;
}

void Server::run()
{
    std::string tCommand;

    while (std::cin >> tCommand)
    {
        if (tCommand == "exit")
        {
            break;
        }
        else if (tCommand == "status")
        {
            std::cout << "[Status] " << mNetworkManager.getSessionRegistry().size() << std::endl;
        }
        else
        {
            std::cout << "[Error] Bilinmeyen komut." << std::endl;
        }
    }
}
