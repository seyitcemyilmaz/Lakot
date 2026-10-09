#include "AccountRepository.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <syncstream>

#include "../../security/PasswordHasher.h"

using namespace lakot;

namespace
{
    // The duplicate check below asks Postgres for a case-insensitive match,
    // so working out WHICH column collided has to compare the same way -
    // otherwise a row that matched on LOWER(email) but differs in case from
    // the submitted address falls through every branch and the INSERT hits
    // the UNIQUE constraint instead, surfacing as a generic database error
    // rather than "email in use".
    bool equalsIgnoreCase(const std::string& pLeft, const std::string& pRight)
    {
        return pLeft.size() == pRight.size()
            && std::equal(pLeft.begin(), pLeft.end(), pRight.begin(),
                          [](unsigned char pA, unsigned char pB)
                          {
                              return std::tolower(pA) == std::tolower(pB);
                          });
    }
}

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
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        )");

        // Position moved to the characters table. Dropped rather than left in
        // place so there is exactly one answer to "where is this player",
        // instead of a stale second copy that nothing writes any more.
        // Existing accounts survive; only their (now meaningless) position
        // columns go.
        tWork.exec(R"(
            ALTER TABLE accounts
                DROP COLUMN IF EXISTS map_id,
                DROP COLUMN IF EXISTS pos_x,
                DROP COLUMN IF EXISTS pos_y,
                DROP COLUMN IF EXISTS pos_z,
                DROP COLUMN IF EXISTS yaw;
        )");

        // Chosen once, with the account's first character. NULL until then.
        tWork.exec("ALTER TABLE accounts ADD COLUMN IF NOT EXISTS kingdom SMALLINT CHECK (kingdom BETWEEN 1 AND 3);");

        tWork.commit();

        std::osyncstream(std::cout) << "[AccountRepo] Tablo hazir/kontrol edildi." << std::endl;
    }, kSchemaAffinityKey);
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

            // Both sides lowered on both columns. The previous form lowered
            // the INPUT for email but the STORED value for username, so the
            // two columns disagreed about what "already taken" means -
            // registering "BOB" while "bob" existed passed the check and then
            // failed on the UNIQUE index.
            std::string tCheckQuery = "SELECT username, email FROM accounts WHERE LOWER(email) = LOWER($1) OR LOWER(username) = LOWER($2)";
            pqxx::result tCheckResult = tWork.exec(tCheckQuery, pqxx::params{pEmail, pUsername});

            for (auto const& tRow : tCheckResult)
            {
                if (equalsIgnoreCase(tRow["email"].as<std::string>(), pEmail))
                {
                    pCallback(RegisterErrorType::eEmailInUse, "");
                    return;
                }

                if (equalsIgnoreCase(tRow["username"].as<std::string>(), pUsername))
                {
                    pCallback(RegisterErrorType::eUsernameInUse, "");
                    return;
                }
            }

            std::string tHashedPassword = PasswordHasher::hash(pPassword);

            if (tHashedPassword.empty())
            {
                // OpenSSL failed (see PasswordHasher::hash) - storing an
                // empty hash would create an account nobody can ever log in
                // to, so refuse the registration instead.
                pCallback(RegisterErrorType::eSystemError, "Password hashing failed.");
                return;
            }

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
        uint64_t tAccountId = 0;

        try
        {
            pqxx::nontransaction tNtx(pConnection);

            // Parameterized like every other query in this class - the old
            // esc()-and-concatenate form was safe but was the only place the
            // escaping correctness depended on remembering to call esc().
            auto tResult = tNtx.exec(
                "SELECT id, password FROM accounts WHERE username = $1",
                pqxx::params{pUsername});

            if (!tResult.empty())
            {
                std::string tDatabasePassword = tResult[0]["password"].as<std::string>();

                if (PasswordHasher::verify(pPassword, tDatabasePassword))
                {
                    tAccountId = tResult[0]["id"].as<uint64_t>();
                    tSuccess = true;
                }
            }
        }
        catch (const std::exception& tException)
        {
            std::osyncstream(std::cerr) << "[AccountRepo] Login Hatasi: " << tException.what() << std::endl;
        }

        if (pCallback)
        {
            pCallback(tSuccess, tAccountId);
        }
    }, std::hash<std::string>{}(pUsername));
}

