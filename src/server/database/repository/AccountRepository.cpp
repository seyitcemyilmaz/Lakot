#include "AccountRepository.h"

#include <iostream>

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
            CREATE TABLE IF NOT EXISTS Account (
                id BIGSERIAL PRIMARY KEY,
                username VARCHAR(50) UNIQUE NOT NULL,
                password VARCHAR(100) NOT NULL,
                email VARCHAR(100) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
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

            std::string tInsertQuery = "INSERT INTO accounts (username, password, email) VALUES ($1, $2, $3)";
            tWork.exec(tInsertQuery, pqxx::params{pUsername, pPassword, pEmail});

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
    });
}

void AccountRepository::findByUsername(const std::string& pUsername, const std::string& pPassword, LoginCallback pCallback)
{
    mDatabaseManager.executeAsync([pUsername, pPassword, pCallback](pqxx::connection& pConnection)
    {
        bool tSuccess = false;
        uint64_t tUserId = 0;

        try
        {
            pqxx::nontransaction tNtx(pConnection);
            std::string tSafeUser = tNtx.esc(pUsername);

            auto tResult = tNtx.exec("SELECT id, password FROM accounts WHERE username = '" + tSafeUser + "'");

            if (!tResult.empty())
            {
                std::string tDatabasePassword = tResult[0]["password"].as<std::string>();

                if (tDatabasePassword == pPassword)
                {
                    tUserId = tResult[0]["id"].as<uint64_t>();
                    tSuccess = true;
                }
            }
        }
        catch (const std::exception& tException)
        {
            std::cerr << "[AccountRepo] Login Hatasi: " << tException.what() << std::endl;
        }

        if (pCallback)
        {
            pCallback(tSuccess, tUserId);
        }
    });
}
