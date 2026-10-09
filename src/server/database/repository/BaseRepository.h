#ifndef LAKOT_SERVER_BASEREPOSITORY_H
#define LAKOT_SERVER_BASEREPOSITORY_H

#include "../DatabaseManager.h"

namespace lakot
{

class BaseRepository
{
public:
    virtual ~BaseRepository() = default;

    explicit BaseRepository(DatabaseManager& pDatabaseManager)
        : mDatabaseManager(pDatabaseManager)
    {

    }

    virtual void initializeTable() = 0;

protected:
    // Every initializeTable() must post with THIS affinity key, never with
    // the round-robin overload. Schema statements have dependencies between
    // repositories (characters carries a foreign key onto accounts), and
    // round-robin would hand them to different workers to run concurrently -
    // so the dependent CREATE could reach the database before the table it
    // references exists. One shared key puts them on one connection, where
    // they execute in the order RepositoryManager::initialize() queues them.
    static constexpr uint64_t kSchemaAffinityKey = 0;

    DatabaseManager& mDatabaseManager;
};

}

#endif
