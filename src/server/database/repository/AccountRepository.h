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

    // Where a player was last known to be - restored on login, saved on
    // disconnect. Deliberately not WorldRegistry::PlayerState - this stays
    // a plain login-response payload shape, keeping database/repository
    // decoupled from the world subsystem.
    struct PlayerSpawnState
    {
        uint32_t mapId = 0;
        float x = 0.0f;
        float y = 2.0f;
        float z = 10.0f;
        float yaw = 0.0f;
    };

    using RegisterCallback = std::function<void(RegisterErrorType, const std::string&)>;
    using LoginCallback = std::function<void(bool, uint64_t, const PlayerSpawnState&)>;

    virtual ~AccountRepository();
    explicit AccountRepository(DatabaseManager& pDatabaseManager);

    void initializeTable() override;

    void createAccount(const std::string& pUsername,
                       const std::string& pPassword,
                       const std::string& pEmail,
                       RegisterCallback pCallback);

    void findByUsername(const std::string& pUsername, const std::string& pPassword, LoginCallback pCallback);

    void savePlayerState(uint64_t pUserId, uint32_t pMapId, float pX, float pY, float pZ, float pYaw);

};

}

#endif
