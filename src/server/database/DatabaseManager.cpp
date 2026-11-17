#include "DatabaseManager.h"

#include <iostream>
#include <algorithm>
#include <thread>
#include <syncstream>

using namespace lakot;

namespace
{
    constexpr std::chrono::seconds kReconnectCooldown{1};
    constexpr std::chrono::seconds kOutageLogInterval{10};
}

DatabaseManager::~DatabaseManager()
{
    stop();
}

DatabaseManager::DatabaseManager()
{

}

void DatabaseManager::initialize(const DatabaseConfig& pConfig)
{
    mConfig = pConfig;

    std::osyncstream(std::cout) << "[Database] Checking the database..." << std::endl;
    ensureDatabaseExists(pConfig);

    unsigned int tPoolSize = pConfig.poolSize > 0
        ? pConfig.poolSize
        : std::clamp(std::thread::hardware_concurrency(), 2u, 8u);

    std::osyncstream(std::cout) << "[Database] Connection pool size: " << tPoolSize << std::endl;

    for (unsigned int tIndex = 0; tIndex < tPoolSize; ++tIndex)
    {
        auto tWorker = std::make_unique<Worker>();
        Worker* tWorkerPtr = tWorker.get();

        tWorker->thread = std::thread([tWorkerPtr]()
        {
            tWorkerPtr->context.run();
        });

        // Posted here (on the calling thread, before any executeAsync() call
        // can reach this worker) so it is always the first task this
        // worker's context executes - mirrors the single-connection
        // version's connect-before-query guarantee, per worker.
        boost::asio::post(tWorkerPtr->context, [tWorkerPtr, tIndex, pConfig]()
        {
            try
            {
                tWorkerPtr->connection = std::make_unique<pqxx::connection>(pConfig.toString());

                if (tWorkerPtr->connection->is_open())
                {
                    std::osyncstream(std::cout) << "[Database] Worker #" << tIndex << " connection OK: " << tWorkerPtr->connection->dbname() << std::endl;
                }
            }
            catch (const std::exception& tException)
            {
                std::osyncstream(std::cerr) << "[Database] Worker #" << tIndex << " critical startup error: " << tException.what() << std::endl;
            }
        });

        mWorkers.push_back(std::move(tWorker));
    }
}

void DatabaseManager::executeAsync(DatabaseTask pTask)
{
    if (mWorkers.empty())
    {
        std::osyncstream(std::cerr) << "[Database] Error: pool is not ready, query was not run." << std::endl;
        return;
    }

    size_t tIndex = mRoundRobinCounter.fetch_add(1, std::memory_order_relaxed) % mWorkers.size();
    postToWorker(tIndex, std::move(pTask), nullptr);
}

void DatabaseManager::executeAsync(DatabaseTask pTask, uint64_t pAffinityKey)
{
    executeAsync(std::move(pTask), pAffinityKey, nullptr);
}

void DatabaseManager::executeAsync(DatabaseTask pTask, uint64_t pAffinityKey, DroppedHandler pOnDropped)
{
    if (mWorkers.empty())
    {
        std::osyncstream(std::cerr) << "[Database] Error: pool is not ready, query was not run." << std::endl;

        if (pOnDropped)
        {
            pOnDropped();
        }

        return;
    }

    size_t tIndex = static_cast<size_t>(pAffinityKey % mWorkers.size());
    postToWorker(tIndex, std::move(pTask), std::move(pOnDropped));
}

void DatabaseManager::postToWorker(size_t pWorkerIndex, DatabaseTask pTask, DroppedHandler pOnDropped)
{
    Worker* tWorker = mWorkers[pWorkerIndex].get();

    boost::asio::post(tWorker->context, [this, tWorker, pWorkerIndex, pTask, pOnDropped]()
    {
        if (ensureConnection(*tWorker, pWorkerIndex))
        {
            try
            {
                pTask(*tWorker->connection);
            }
            catch (const std::exception& tException)
            {
                std::osyncstream(std::cerr) << "[Database] Worker #" << pWorkerIndex << " query error: " << tException.what() << std::endl;
            }
            catch (...)
            {
                std::osyncstream(std::cerr) << "[Database] Worker #" << pWorkerIndex << " query error: unknown error" << std::endl;
            }
        }
        else
        {
            if (pOnDropped)
            {
                pOnDropped();
            }
        }
    });
}

bool DatabaseManager::ensureConnection(Worker& pWorker, size_t pWorkerIndex)
{
    if (pWorker.connection && pWorker.connection->is_open())
    {
        try
        {
            pWorker.connection->get_notifs();
            return true;
        }
        catch (const pqxx::broken_connection& tException)
        {
            std::osyncstream(std::cerr) << "[Database] Worker #" << pWorkerIndex << " connection lost: " << tException.what() << std::endl;
            pWorker.connection.reset();
        }
    }

    const auto tNow = std::chrono::steady_clock::now();

    if (pWorker.hasConnectFailed && tNow - pWorker.lastConnectFailureAt < kReconnectCooldown)
    {
        return false;
    }

    try
    {
        pWorker.connection = std::make_unique<pqxx::connection>(mConfig.toString());

        if (pWorker.connection->is_open())
        {
            pWorker.hasConnectFailed = false;
            pWorker.hasLoggedOutage = false;
            std::osyncstream(std::cout) << "[Database] Worker #" << pWorkerIndex << " reconnected." << std::endl;
            return true;
        }
    }
    catch (const std::exception& tException)
    {
        const auto tFailedAt = std::chrono::steady_clock::now();

        if (!pWorker.hasLoggedOutage || tFailedAt - pWorker.lastOutageLogAt >= kOutageLogInterval)
        {
            pWorker.hasLoggedOutage = true;
            pWorker.lastOutageLogAt = tFailedAt;
            std::osyncstream(std::cerr) << "[Database] Worker #" << pWorkerIndex << " reconnect error: " << tException.what() << std::endl;
        }
    }

    pWorker.hasConnectFailed = true;
    pWorker.lastConnectFailureAt = std::chrono::steady_clock::now();
    return false;
}

void DatabaseManager::ensureDatabaseExists(const DatabaseConfig& pConfig)
{
    try
    {
        pqxx::connection tSysConn(pConfig.toSystemString());
        pqxx::nontransaction tNtx(tSysConn);

        std::string tCheckQuery = "SELECT 1 FROM pg_database WHERE datname = '" + tNtx.esc(pConfig.name) + "'";
        pqxx::result tRes = tNtx.exec(tCheckQuery);

        if (tRes.empty())
        {
            std::osyncstream(std::cout) << "[Database] Creating: " << pConfig.name << std::endl;
            tNtx.exec("CREATE DATABASE " + tNtx.quote_name(pConfig.name));
            std::osyncstream(std::cout) << "[Database] Created." << std::endl;
        }
    }
    catch (const std::exception& tException)
    {
        std::string tErr = tException.what();
        if (tErr.find("already exists") == std::string::npos)
        {
            std::osyncstream(std::cerr) << "[Database] CreateDB warning: " << tException.what() << std::endl;
        }
    }
}

void DatabaseManager::stop()
{
    // Releasing the work guard (rather than calling context.stop()) lets each
    // worker finish the tasks already queued on it and then return from run()
    // on its own. context.stop() would abandon them - which silently dropped
    // the position saves posted during shutdown, the very writes that most
    // need to land. Nothing new can be posted at this point: the network
    // layer is already stopped by the time Server's destructor gets here.
    for (auto& tWorker : mWorkers)
    {
        tWorker->workGuard.reset();
    }

    for (auto& tWorker : mWorkers)
    {
        if (tWorker->thread.joinable())
        {
            tWorker->thread.join();
        }
    }

    // stop() runs from both Server's destructor and DatabaseManager's own -
    // clearing the pool makes the second call a no-op instead of re-joining
    // already-joined threads.
    mWorkers.clear();
}

bool DatabaseManager::isConnected() const
{
    if (mWorkers.empty())
    {
        return false;
    }

    for (const auto& tWorker : mWorkers)
    {
        if (!tWorker->connection || !tWorker->connection->is_open())
        {
            return false;
        }
    }

    return true;
}
