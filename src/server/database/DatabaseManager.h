#ifndef LAKOT_SERVER_DATABASEMANAGER_H
#define LAKOT_SERVER_DATABASEMANAGER_H

#include <functional>
#include <vector>
#include <atomic>

#include <pqxx/pqxx>
#include <boost/asio.hpp>

#include "DatabaseConfig.h"

namespace lakot
{

class DatabaseManager
{
public:
    using DatabaseTask = std::function<void(pqxx::connection&)>;

    virtual ~DatabaseManager();
    DatabaseManager();

    void initialize(const DatabaseConfig& pConfig);
    void stop();

    // Round-robin across the pool. Use this when the task has no natural
    // entity to serialize against (e.g. it only ever touches its own new row).
    void executeAsync(DatabaseTask pTask);

    // Pins every call sharing the same pAffinityKey (e.g. a hash of a
    // username or character id) to the same worker/connection, so they
    // always execute in submission order relative to each other, while
    // different keys still run in parallel across the rest of the pool.
    void executeAsync(DatabaseTask pTask, uint64_t pAffinityKey);

    bool isConnected() const;

private:
    struct Worker
    {
        boost::asio::io_context context;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard;
        std::thread thread;
        std::unique_ptr<pqxx::connection> connection;

        Worker()
            : workGuard(boost::asio::make_work_guard(context))
        {

        }
    };

    std::vector<std::unique_ptr<Worker>> mWorkers;
    std::atomic<uint64_t> mRoundRobinCounter{0};

    void postToWorker(size_t pWorkerIndex, DatabaseTask pTask);
    void ensureDatabaseExists(const DatabaseConfig& pConfig);
};

}

#endif
