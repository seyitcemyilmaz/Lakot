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
    enum class RegisterErrorType
    {
        eNoError,
        eEmailInUse,
        eUsernameInUse,
        eDatabaseError,
        eSystemError
    };

    using RegisterCallback = std::function<void(RegisterErrorType, const std::string&)>;
    using LoginCallback = std::function<void(bool, uint64_t)>;

    virtual ~AccountRepository();
    explicit AccountRepository(DatabaseManager& pDatabaseManager);

    void initializeTable() override;

    void createAccount(const std::string& pUsername,
                       const std::string& pPassword,
                       const std::string& pEmail,
                       RegisterCallback pCallback);

    void findByUsername(const std::string& pUsername, const std::string& pPassword, LoginCallback pCallback);

};

}

#endif
