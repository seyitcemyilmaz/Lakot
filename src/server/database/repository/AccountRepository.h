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

    // Authentication only - an account has no position. Where a player stands
    // belongs to a character (see CharacterRepository), which is chosen after
    // logging in rather than being implied by the login itself.
    using LoginCallback = std::function<void(bool pIsSuccess, uint64_t pAccountId)>;

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
