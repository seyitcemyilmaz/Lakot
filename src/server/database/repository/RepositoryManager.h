#ifndef LAKOT_SERVER_REPOSITORYMANAGER_H
#define LAKOT_SERVER_REPOSITORYMANAGER_H

#include "AccountRepository.h"

namespace lakot
{

class DatabaseManager;

class RepositoryManager
{
public:
    virtual ~RepositoryManager();
    explicit RepositoryManager(DatabaseManager& pDatabaseManager);

    void initialize();

    AccountRepository& getAccountRepository() const;

private:
    std::unique_ptr<AccountRepository> mAccountRepository;
};

}

#endif
