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
    DatabaseManager& mDatabaseManager;
};

}

#endif
