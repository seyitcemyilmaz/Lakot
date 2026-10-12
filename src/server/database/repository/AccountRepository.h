#ifndef LAKOT_SERVER_ACCOUNTREPOSITORY_H
#define LAKOT_SERVER_ACCOUNTREPOSITORY_H

#include <cstdint>
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

    enum class LoginResultType
    {
        eSuccess,
        eInvalidCredentials,
        eDatabaseError
    };

    using LoginCallback = std::function<void(LoginResultType pResult, uint64_t pAccountId)>;

    enum class MuteResultType
    {
        eApplied,
        eNotFound,
        eProtected
    };

    struct MuteOutcome
    {
        MuteResultType result = MuteResultType::eNotFound;
        std::string characterName;
        uint64_t accountId = 0;
        int64_t mutedUntilEpoch = 0;
    };

    using MuteCallback = std::function<void(const MuteOutcome&)>;
    using GrantGmCallback = std::function<void(bool pIsFound)>;

    virtual ~AccountRepository();
    explicit AccountRepository(DatabaseManager& pDatabaseManager);

    void initializeTable() override;

    void createAccount(const std::string& pUsername,
                       const std::string& pPassword,
                       const std::string& pEmail,
                       RegisterCallback pCallback);

    void findByUsername(const std::string& pUsername, const std::string& pPassword, LoginCallback pCallback);

    void grantGmLevel(const std::string& pUsername, uint32_t pLevel, GrantGmCallback pCallback);

    void muteByCharacterName(const std::string& pCharacterName, uint32_t pMinutes,
                             uint32_t pActorLevel, uint64_t pActorAccountId, MuteCallback pCallback);

};

}

#endif
