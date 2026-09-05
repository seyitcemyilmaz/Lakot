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
    mRepositoryManager = std::make_unique<RepositoryManager>(mDatabaseManager);

    mAuthController = std::make_unique<AuthController>(mNetworkManager, *mRepositoryManager);
    mWorldController = std::make_unique<WorldController>(mNetworkManager, *mRepositoryManager);
    mChatController = std::make_unique<ChatController>(mNetworkManager, *mRepositoryManager);

    mAuthController->setWorldController(*mWorldController);
    mChatController->setWorldController(*mWorldController);
}

bool Server::initialize()
{
    std::cout << "--- Lakot Game Server Baslatiliyor ---" << std::endl;

    DatabaseConfig tDatabaseConfig;

    mDatabaseManager.initialize(tDatabaseConfig);
    mRepositoryManager->initialize();

    mAuthController->initialize();
    mWorldController->initialize();
    mChatController->initialize();

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
