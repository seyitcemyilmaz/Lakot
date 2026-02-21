#ifndef LAKOT_SERVER_ACCOUNTREPOSITORY_H
#define LAKOT_SERVER_ACCOUNTREPOSITORY_H

#include <string>
#include <functional>

#include "BaseRepository.h"

namespace lakot
{

class AccountRepository : public BaseRepository
{
public:
    using LoginCallback = std::function<void(bool, uint64_t)>;

    virtual ~AccountRepository();
    explicit AccountRepository(DatabaseManager& pDatabaseManager);

    void initializeTable() override;

    void findByUsername(const std::string& pUsername, const std::string& pPassword, LoginCallback pCallback);

    void create(const std::string& pUsername, const std::string& pPassword);
};

}

#endif
