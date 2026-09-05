#include "AccountRepository.h"

#include <iostream>

#include "../../security/PasswordHasher.h"

using namespace lakot;

AccountRepository::~AccountRepository()
{

}

AccountRepository::AccountRepository(DatabaseManager& pDatabaseManager)
    : BaseRepository(pDatabaseManager)
{
}

void AccountRepository::initializeTable()
{
    mDatabaseManager.executeAsync([](pqxx::connection& pConnection)
    {
        pqxx::work tWork(pConnection);

        tWork.exec(R"(
            CREATE TABLE IF NOT EXISTS accounts (
                id BIGSERIAL PRIMARY KEY,
                username VARCHAR(50) UNIQUE NOT NULL,
                password VARCHAR(255) NOT NULL,
                email VARCHAR(100) UNIQUE NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                map_id INTEGER NOT NULL DEFAULT 0,
                pos_x REAL NOT NULL DEFAULT 0,
                pos_y REAL NOT NULL DEFAULT 2,
                pos_z REAL NOT NULL DEFAULT 10,
                yaw REAL NOT NULL DEFAULT 0
            );
        )");

        tWork.commit();

        std::cout << "[AccountRepo] Tablo hazir/kontrol edildi." << std::endl;
    });
}

void AccountRepository::createAccount(const std::string& pUsername,
                                      const std::string& pPassword,
                                      const std::string& pEmail,
                                      RegisterCallback pCallback)
{
    // Same username always routes to the same worker, so two concurrent
    // registration attempts for it serialize instead of racing the
    // check-then-insert below across two different connections.
    mDatabaseManager.executeAsync(
    [this, pUsername, pPassword, pEmail, pCallback](pqxx::connection& pConnection)
    {
        try
        {
            pqxx::work tWork(pConnection);

            std::string tCheckQuery = "SELECT username, email FROM accounts WHERE email = LOWER($1) OR LOWER(username) = $2";
            pqxx::result tCheckResult = tWork.exec(tCheckQuery, pqxx::params{pEmail, pUsername});

            if (!tCheckResult.empty())
            {
                for (auto const& tRow : tCheckResult)
                {
                    if (tRow["email"].as<std::string>() == pEmail)
                    {
                        pCallback(RegisterErrorType::eEmailInUse, "");
                        return;
                    }

                    if (tRow["username"].as<std::string>() == pUsername)
                    {
                        pCallback(RegisterErrorType::eUsernameInUse, "");
                        return;
                    }
                }
            }

            std::string tHashedPassword = PasswordHasher::hash(pPassword);

            std::string tInsertQuery = "INSERT INTO accounts (username, password, email) VALUES ($1, $2, $3)";
            tWork.exec(tInsertQuery, pqxx::params{pUsername, tHashedPassword, pEmail});

            tWork.commit();

            pCallback(RegisterErrorType::eNoError, "Registration is completed successfully.");
        }
        catch (const pqxx::sql_error& tException)
        {
            pCallback(RegisterErrorType::eDatabaseError, tException.what());
        }
        catch (const std::exception& tException)
        {
            pCallback(RegisterErrorType::eSystemError, tException.what());
        }
    }, std::hash<std::string>{}(pUsername));
}

void AccountRepository::findByUsername(const std::string& pUsername, const std::string& pPassword, LoginCallback pCallback)
{
    // Same affinity key as createAccount, so a user's own operations always
    // land on the same worker as more per-account mutations get added later.
    mDatabaseManager.executeAsync([pUsername, pPassword, pCallback](pqxx::connection& pConnection)
    {
        bool tSuccess = false;
        uint64_t tUserId = 0;
        PlayerSpawnState tSpawnState;

        try
        {
            pqxx::nontransaction tNtx(pConnection);
            std::string tSafeUser = tNtx.esc(pUsername);

            auto tResult = tNtx.exec("SELECT id, password, map_id, pos_x, pos_y, pos_z, yaw FROM accounts WHERE username = '" + tSafeUser + "'");

            if (!tResult.empty())
            {
                std::string tDatabasePassword = tResult[0]["password"].as<std::string>();

                if (PasswordHasher::verify(pPassword, tDatabasePassword))
                {
                    tUserId = tResult[0]["id"].as<uint64_t>();
                    tSuccess = true;

                    tSpawnState.mapId = tResult[0]["map_id"].as<uint32_t>();
                    tSpawnState.x = tResult[0]["pos_x"].as<float>();
                    tSpawnState.y = tResult[0]["pos_y"].as<float>();
                    tSpawnState.z = tResult[0]["pos_z"].as<float>();
                    tSpawnState.yaw = tResult[0]["yaw"].as<float>();
                }
            }
        }
        catch (const std::exception& tException)
        {
            std::cerr << "[AccountRepo] Login Hatasi: " << tException.what() << std::endl;
        }

        if (pCallback)
        {
            pCallback(tSuccess, tUserId, tSpawnState);
        }
    }, std::hash<std::string>{}(pUsername));
}

void AccountRepository::savePlayerState(uint64_t pUserId, uint32_t pMapId, float pX, float pY, float pZ, float pYaw)
{
    mDatabaseManager.executeAsync([pUserId, pMapId, pX, pY, pZ, pYaw](pqxx::connection& pConnection)
    {
        try
        {
            pqxx::work tWork(pConnection);

            tWork.exec(
                "UPDATE accounts SET map_id=$1, pos_x=$2, pos_y=$3, pos_z=$4, yaw=$5 WHERE id=$6",
                pqxx::params{pMapId, pX, pY, pZ, pYaw, pUserId});

            tWork.commit();
        }
        catch (const std::exception& tException)
        {
            std::cerr << "[AccountRepo] Konum kaydi hatasi: " << tException.what() << std::endl;
        }
    });
}
