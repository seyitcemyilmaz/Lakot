#ifndef LAKOT_SERVER_DATABASEMANAGER_H
#define LAKOT_SERVER_DATABASEMANAGER_H

#include <functional>

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

    void executeAsync(DatabaseTask pTask);

    bool isConnected() const;

private:
    boost::asio::io_context mDatabaseContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> mWorkGuard;
    std::thread mDatabaseThread;

    std::unique_ptr<pqxx::connection> mDatabaseConnection;

    void runWorker();
    void ensureDatabaseExists(const DatabaseConfig& pConfig);
};

}

#endif
