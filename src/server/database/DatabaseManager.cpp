#include "DatabaseManager.h"

#include <iostream>
#include <algorithm>
#include <thread>

using namespace lakot;

DatabaseManager::~DatabaseManager()
{
    stop();
}

DatabaseManager::DatabaseManager()
{

}

void DatabaseManager::initialize(const DatabaseConfig& pConfig)
{
    std::cout << "[Database] Veritabani kontrol ediliyor..." << std::endl;
    ensureDatabaseExists(pConfig);

    unsigned int tPoolSize = pConfig.poolSize > 0
        ? pConfig.poolSize
        : std::clamp(std::thread::hardware_concurrency(), 2u, 8u);

    std::cout << "[Database] Connection pool boyutu: " << tPoolSize << std::endl;

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
                    std::cout << "[Database] Worker #" << tIndex << " baglanti BASARILI: " << tWorkerPtr->connection->dbname() << std::endl;
                }
            }
            catch (const std::exception& tException)
            {
                std::cerr << "[Database] Worker #" << tIndex << " Kritik Baslatma Hatasi: " << tException.what() << std::endl;
            }
        });

        mWorkers.push_back(std::move(tWorker));
    }
}

void DatabaseManager::executeAsync(DatabaseTask pTask)
{
    if (mWorkers.empty())
    {
        std::cerr << "[Database] Hata: Havuz hazir degil, sorgu calistirilamadi." << std::endl;
        return;
    }

    size_t tIndex = mRoundRobinCounter.fetch_add(1, std::memory_order_relaxed) % mWorkers.size();
    postToWorker(tIndex, std::move(pTask));
}

void DatabaseManager::executeAsync(DatabaseTask pTask, uint64_t pAffinityKey)
{
    if (mWorkers.empty())
    {
        std::cerr << "[Database] Hata: Havuz hazir degil, sorgu calistirilamadi." << std::endl;
        return;
    }

    size_t tIndex = static_cast<size_t>(pAffinityKey % mWorkers.size());
    postToWorker(tIndex, std::move(pTask));
}

void DatabaseManager::postToWorker(size_t pWorkerIndex, DatabaseTask pTask)
{
    Worker* tWorker = mWorkers[pWorkerIndex].get();

    boost::asio::post(tWorker->context, [tWorker, pWorkerIndex, pTask]()
    {
        if (tWorker->connection && tWorker->connection->is_open())
        {
            try
            {
                pTask(*tWorker->connection);
            }
            catch (const std::exception& tException)
            {
                std::cerr << "[Database] Worker #" << pWorkerIndex << " Sorgu Hatasi: " << tException.what() << std::endl;
            }
        }
        else
        {
            std::cerr << "[Database] Worker #" << pWorkerIndex << " Hata: Baglanti yok, sorgu calistirilamadi." << std::endl;
        }
    });
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
            std::cout << "[Database] Olusturuluyor: " << pConfig.name << std::endl;
            tNtx.exec("CREATE DATABASE " + pConfig.name);
            std::cout << "[Database] Olusturuldu." << std::endl;
        }
    }
    catch (const std::exception& tException)
    {
        std::string tErr = tException.what();
        if (tErr.find("already exists") == std::string::npos)
        {
            std::cerr << "[Database] CreateDB Uyarisi: " << tException.what() << std::endl;
        }
    }
}

void DatabaseManager::stop()
{
    for (auto& tWorker : mWorkers)
    {
        tWorker->context.stop();
    }

    for (auto& tWorker : mWorkers)
    {
        if (tWorker->thread.joinable())
        {
            tWorker->thread.join();
        }
    }
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
