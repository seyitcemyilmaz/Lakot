#include "RepositoryManager.h"

using namespace lakot;

RepositoryManager::~RepositoryManager()
{

}

RepositoryManager::RepositoryManager(DatabaseManager& pDatabaseManager)
{
    mAccountRepository = std::make_unique<AccountRepository>(pDatabaseManager);
}

void RepositoryManager::initialize()
{
    mAccountRepository->initializeTable();
}

AccountRepository& RepositoryManager::getAccountRepository() const
{
    return *mAccountRepository;
}
