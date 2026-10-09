#include "Server.h"

#include <chrono>
#include <csignal>
#include <future>
#include <thread>
#include <vector>
#include <syncstream>

using namespace lakot;

namespace
{
    // Set from a signal handler, so it must be the async-signal-safe type and
    // nothing else may happen in there - the handler only flips this and the
    // condition variable below is notified from the polling wait in
    // waitForShutdownSignal().
    volatile std::sig_atomic_t gShutdownRequested = 0;

    void onShutdownSignal(int)
    {
        gShutdownRequested = 1;
    }

    // Blocks until Ctrl+C / SIGTERM. Used when there is no interactive
    // console to read commands from.
    void waitForShutdownSignal()
    {
        std::signal(SIGINT, onShutdownSignal);
        std::signal(SIGTERM, onShutdownSignal);

        while (gShutdownRequested == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
}

Server::~Server()
{
    // Network first, so no new gameplay work can arrive while the remaining
    // player state is being flushed.
    mNetworkManager.stop();

    // Stops every zone thread, which flushes all tracked positions through to
    // the database queue on the way out.
    if (mWorldController)
    {
        mWorldController->shutdown();
    }

    // Drains the queue those saves just posted before joining - see
    // DatabaseManager::stop().
    mDatabaseManager.stop();
}

Server::Server()
    : mDatabaseManager()
    , mNetworkManager()
{
    mRepositoryManager = std::make_unique<RepositoryManager>(mDatabaseManager);
}

bool Server::initialize()
{
    std::osyncstream(std::cout) << "--- Lakot Game Server Baslatiliyor ---" << std::endl;

    DatabaseConfig tDatabaseConfig;

    if (tDatabaseConfig.password.empty())
    {
        std::osyncstream(std::cerr) << "[Server] LAKOT_DB_PASSWORD ortam degiskeni tanimli degil." << std::endl;
        return false;
    }

    mDatabaseManager.initialize(tDatabaseConfig);
    mRepositoryManager->initialize();

    // Blocking on purpose, and the one place in the server that does.
    //
    // The catalog is read without a lock by every zone thread precisely
    // because it never changes after startup - so it has to be COMPLETE
    // before anything can read it. Starting the network first and filling it
    // asynchronously would mean a player could enter the world, and a zone
    // could look up an item, while the table was still half-loaded. Startup
    // is allowed to take a moment; a data race is not.
    std::string tItemError;

    if (!mItemCatalog.load("data", tItemError))
    {
        std::osyncstream(std::cerr) << "[Server] Esya katalogu yuklenemedi: " << tItemError << std::endl;
        return false;
    }

    {
        std::promise<std::pair<bool, uint64_t>> tPromise;
        std::future<std::pair<bool, uint64_t>> tFuture = tPromise.get_future();

        mRepositoryManager->getItemRepository().syncTemplates(mItemCatalog.getAll(),
        [&tPromise](bool pIsSuccess, uint64_t pRemovedInstances)
        {
            tPromise.set_value({ pIsSuccess, pRemovedInstances });
        });

        auto [tIsSynced, tRemoved] = tFuture.get();

        if (!tIsSynced)
        {
            return false;
        }

        std::osyncstream(std::cout) << "[Server] Esya katalogu yuklendi: " << mItemCatalog.size() << " sablon, "
                                    << tRemoved << " eski esya silindi." << std::endl;
    }

    // After the catalog is loaded, before any controller starts a zone or
    // builds a character from it.
    std::string tMapError;

    if (!mMapCatalog.load("data", tMapError))
    {
        std::osyncstream(std::cerr) << "[Server] Harita verisi yuklenemedi: " << tMapError << std::endl;
        return false;
    }

    std::osyncstream(std::cout) << "[Server] Harita verisi yuklendi: " << mMapCatalog.getMapIds().size() << " harita, "
              << mMapCatalog.getKingdoms().size() << " krallik." << std::endl;

    if (!mMonsterCatalog.load("data", tMapError))
    {
        std::osyncstream(std::cerr) << "[Server] Canavar verisi yuklenemedi: " << tMapError << std::endl;
        return false;
    }

    for (const auto& [tMonsterId, tMonster] : mMonsterCatalog.getAll())
    {
        for (const MonsterDrop& tDrop : tMonster.drops)
        {
            if (!mItemCatalog.find(tDrop.itemId))
            {
                std::osyncstream(std::cerr) << "[Server] Canavar " << tMonsterId << " bilinmeyen esya dusuruyor: " << tDrop.itemId << std::endl;
            }
        }
    }

    RepositoryManager& tRepositories = *mRepositoryManager;

    mWorldController = std::make_unique<WorldController>(mNetworkManager, tRepositories, mItemCatalog, mMapCatalog, mMonsterCatalog);
    mInventoryController = std::make_unique<InventoryController>(mNetworkManager, tRepositories, *mWorldController, mItemCatalog);
    mChatController = std::make_unique<ChatController>(mNetworkManager, tRepositories, *mWorldController);
    mCharacterController = std::make_unique<CharacterController>(mNetworkManager, tRepositories, *mWorldController,
                                                                 *mInventoryController, mItemCatalog, mMapCatalog);
    mAuthController = std::make_unique<AuthController>(mNetworkManager, tRepositories, *mWorldController);

    mWorldController->initialize();
    mInventoryController->initialize();
    mChatController->initialize();
    mCharacterController->initialize();
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
            return;
        }
        else if (tCommand == "status")
        {
            std::osyncstream(std::cout) << "[Status] " << mNetworkManager.getSessionRegistry().size() << std::endl;
        }
        else
        {
            std::osyncstream(std::cout) << "[Error] Bilinmeyen komut." << std::endl;
        }
    }

    // Falling out of the loop means stdin ended rather than "exit" being
    // typed - which is the normal state for a server started as a service,
    // from a scheduler, or with its input redirected. Previously that shut
    // the whole server down within milliseconds of finishing startup, so it
    // could only ever run attached to a live console. Now it keeps serving
    // and waits to be told to stop.
    std::osyncstream(std::cout) << "[Server] Konsol girdisi yok - kapatma sinyali bekleniyor (Ctrl+C)." << std::endl;

    waitForShutdownSignal();

    std::osyncstream(std::cout) << "[Server] Kapatma sinyali alindi." << std::endl;
}
