#include "RepositoryManager.h"

using namespace lakot;

RepositoryManager::~RepositoryManager()
{

}

RepositoryManager::RepositoryManager(DatabaseManager& pDatabaseManager)
{
    mAccountRepository = std::make_unique<AccountRepository>(pDatabaseManager);
    mCharacterRepository = std::make_unique<CharacterRepository>(pDatabaseManager);
    mItemRepository = std::make_unique<ItemRepository>(pDatabaseManager);
}

void RepositoryManager::initialize()
{
    // Order matters: characters carries a foreign key onto accounts, so the
    // accounts table has to exist first. Both initializeTable()s post with
    // BaseRepository::kSchemaAffinityKey, which pins them to one worker and
    // therefore makes this call order the execution order.
    mAccountRepository->initializeTable();
    mCharacterRepository->initializeTable();

    // Last: item_instances references characters, which references accounts.
    mItemRepository->initializeTable();
}

AccountRepository& RepositoryManager::getAccountRepository() const
{
    return *mAccountRepository;
}

ItemRepository& RepositoryManager::getItemRepository() const
{
    return *mItemRepository;
}

CharacterRepository& RepositoryManager::getCharacterRepository() const
{
    return *mCharacterRepository;
}
