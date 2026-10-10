#ifndef LAKOT_SERVER_REPOSITORYMANAGER_H
#define LAKOT_SERVER_REPOSITORYMANAGER_H

#include "AccountRepository.h"
#include "CharacterRepository.h"
#include "ItemRepository.h"
#include "GmLogRepository.h"

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
    CharacterRepository& getCharacterRepository() const;
    ItemRepository& getItemRepository() const;
    GmLogRepository& getGmLogRepository() const;

private:
    std::unique_ptr<AccountRepository> mAccountRepository;
    std::unique_ptr<CharacterRepository> mCharacterRepository;
    std::unique_ptr<ItemRepository> mItemRepository;
    std::unique_ptr<GmLogRepository> mGmLogRepository;
};

}

#endif
