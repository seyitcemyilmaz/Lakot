#include "Server.h"

#include <atomic>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <future>
#include <string_view>
#include <thread>
#include <vector>
#include <syncstream>

using namespace lakot;

namespace
{
    std::atomic<bool> gShutdownRequested{false};
    std::atomic<bool> gStatusRequested{false};

    static_assert(std::atomic<bool>::is_always_lock_free);

    void onShutdownSignal(int)
    {
        gShutdownRequested = true;
    }

    constexpr uint16_t kDefaultServerPort = 12345;
    constexpr std::chrono::seconds kStartupQueryTimeout{60};

    std::optional<uint16_t> readServerPort()
    {
        const char* tValue = std::getenv("LAKOT_SERVER_PORT");

        if (!tValue || !*tValue)
        {
            return kDefaultServerPort;
        }

        const std::string_view tText(tValue);
        unsigned int tPort = 0;
        const auto [tEnd, tError] = std::from_chars(tText.data(), tText.data() + tText.size(), tPort);

        if (tError != std::errc{} || tEnd != tText.data() + tText.size() || tPort < 1 || tPort > 65535)
        {
            return std::nullopt;
        }

        return static_cast<uint16_t>(tPort);
    }

    // Blocks until Ctrl+C / SIGTERM. Used when there is no interactive
    // console to read commands from.
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
    , mServerPort(readServerPort())
    , mNetworkManager(mServerPort.value_or(kDefaultServerPort))
{
    mRepositoryManager = std::make_unique<RepositoryManager>(mDatabaseManager);
}

bool Server::initialize()
{
    std::osyncstream(std::cout) << "--- Lakot Game Server Baslatiliyor ---" << std::endl;

    if (!mServerPort)
    {
        std::osyncstream(std::cerr) << "[Server] LAKOT_SERVER_PORT gecersiz (1-65535 arasi sayi olmali)." << std::endl;
        return false;
    }

    DatabaseConfig tDatabaseConfig;

    if (!tDatabaseConfig.isPortValid())
    {
        std::osyncstream(std::cerr) << "[Server] LAKOT_DB_PORT gecersiz (1-65535 arasi sayi olmali)." << std::endl;
        return false;
    }

    if (tDatabaseConfig.password.empty())
    {
        std::osyncstream(std::cerr) << "[Server] LAKOT_DB_PASSWORD ortam degiskeni tanimli degil." << std::endl;
        return false;
    }

    mDatabaseManager.initialize(tDatabaseConfig);
    mRepositoryManager->initialize();
    grantConfiguredGmAccount();

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
        auto tPromise = std::make_shared<std::promise<std::pair<bool, uint64_t>>>();
        std::future<std::pair<bool, uint64_t>> tFuture = tPromise->get_future();

        mRepositoryManager->getItemRepository().syncTemplates(mItemCatalog.getAll(),
        [tPromise](bool pIsSuccess, uint64_t pRemovedInstances)
        {
            tPromise->set_value({ pIsSuccess, pRemovedInstances });
        });

        if (tFuture.wait_for(kStartupQueryTimeout) != std::future_status::ready)
        {
            std::osyncstream(std::cerr) << "[Server] Esya sablonlari zaman asimi: veritabani yanit vermiyor." << std::endl;
            return false;
        }

        auto [tIsSynced, tRemoved] = tFuture.get();

        if (!tIsSynced)
        {
            std::osyncstream(std::cerr) << "[Server] Esya sablonlari veritabanina yazilamadi." << std::endl;
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
    mCommandController = std::make_unique<CommandController>(mNetworkManager, tRepositories, *mWorldController);
    mWorldCommands = std::make_unique<WorldCommands>(*mWorldController, mItemCatalog, mMonsterCatalog);
    mModerationCommands = std::make_unique<ModerationCommands>(*mWorldController, mNetworkManager, tRepositories.getAccountRepository());
    mChatController = std::make_unique<ChatController>(mNetworkManager, tRepositories, *mWorldController, *mCommandController);
    mCharacterController = std::make_unique<CharacterController>(mNetworkManager, tRepositories, *mWorldController,
                                                                 *mInventoryController, mItemCatalog, mMapCatalog);
    mAuthController = std::make_unique<AuthController>(mNetworkManager, tRepositories, *mWorldController);

    mWorldController->initialize();
    mInventoryController->initialize();
    mCommandController->initialize();
    mWorldCommands->registerCommands(*mCommandController);
    mModerationCommands->registerCommands(*mCommandController);
    mChatController->initialize();
    mCharacterController->initialize();
    mAuthController->initialize();

    mNetworkManager.start();

    return true;
}

void Server::grantConfiguredGmAccount()
{
    const char* tAccountName = std::getenv("LAKOT_GM_ACCOUNT");

    if (!tAccountName || !*tAccountName)
    {
        return;
    }

    std::string tList(tAccountName);
    std::size_t tPos = 0;

    while (tPos <= tList.size())
    {
        std::size_t tEnd = tList.find(',', tPos);

        if (tEnd == std::string::npos)
        {
            tEnd = tList.size();
        }

        std::string tEntry = tList.substr(tPos, tEnd - tPos);
        tPos = tEnd + 1;

        const auto tFirst = tEntry.find_first_not_of(" 	");

        if (tFirst == std::string::npos)
        {
            continue;
        }

        tEntry = tEntry.substr(tFirst, tEntry.find_last_not_of(" 	") - tFirst + 1);

        std::string tName = tEntry;
        int tLevel = 2;
        const auto tColon = tEntry.find(':');

        if (tColon != std::string::npos)
        {
            tName = tEntry.substr(0, tColon);
            const std::string tLevelText = tEntry.substr(tColon + 1);

            if (tLevelText == "1")
            {
                tLevel = 1;
            }
            else if (tLevelText == "2")
            {
                tLevel = 2;
            }
            else
            {
                std::osyncstream(std::cerr) << "[Security] UYARI: LAKOT_GM_ACCOUNT gecersiz seviye, girdi atlandi: " << tEntry << std::endl;
                continue;
            }
        }

        if (tName.empty())
        {
            continue;
        }

        std::osyncstream(std::cerr) << "[Security] UYARI: LAKOT_GM_ACCOUNT tanimli, '" << tName
                                    << "' hesabi GM seviye " << tLevel << " olarak ayarlaniyor." << std::endl;

        mRepositoryManager->getAccountRepository().grantGmLevel(tName, tLevel,
        [tName](bool pIsFound)
        {
            if (!pIsFound)
            {
                std::osyncstream(std::cerr) << "[Security] UYARI: LAKOT_GM_ACCOUNT hesabi bulunamadi: " << tName << std::endl;
            }
        });
    }
}

void Server::requestShutdown()
{
    gShutdownRequested = true;
}

void Server::run()
{
    std::signal(SIGINT, onShutdownSignal);
    std::signal(SIGTERM, onShutdownSignal);

    std::thread([]()
    {
        std::string tCommand;

        while (std::cin >> tCommand)
        {
            if (tCommand == "exit")
            {
                gShutdownRequested = true;
                return;
            }
            else if (tCommand == "status")
            {
                gStatusRequested = true;
            }
            else
            {
                std::osyncstream(std::cout) << "[Error] Bilinmeyen komut." << std::endl;
            }
        }

        if (!gShutdownRequested)
        {
            std::osyncstream(std::cout) << "[Server] Konsol girdisi yok - kapatma sinyali bekleniyor (Ctrl+C)." << std::endl;
        }
    }).detach();

    while (!gShutdownRequested)
    {
        if (gStatusRequested.exchange(false))
        {
            std::osyncstream(std::cout) << "[Status] " << mNetworkManager.getSessionRegistry().size() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::osyncstream(std::cout) << "[Server] Kapatma istegi alindi." << std::endl;
}
