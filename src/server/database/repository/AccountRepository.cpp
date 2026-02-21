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
            CREATE TABLE IF NOT EXISTS accounts (
                id BIGSERIAL PRIMARY KEY,
                username VARCHAR(50) UNIQUE NOT NULL,
                password VARCHAR(100) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        )");

        tWork.commit();

        std::cout << "[AccountRepo] Tablo hazir/kontrol edildi." << std::endl;
    });
}

void AccountRepository::create(const std::string& pUsername, const std::string& pPassword)
{
    mDatabaseManager.executeAsync(
    [pUsername, pPassword](pqxx::connection& pConnection)
    {
        pqxx::work tWork(pConnection);
        // ON CONFLICT: Ayni kullanici varsa hata verme, pas gec
        std::string tQuery = "INSERT INTO accounts (username, password) VALUES ('" +
                             tWork.esc(pUsername) + "', '" + tWork.esc(pPassword) + "') " +
                             "ON CONFLICT (username) DO NOTHING";
        tWork.exec(tQuery);
        tWork.commit();
        // std::cout << "[AccountRepo] Hesap olusturuldu: " << pUsername << std::endl;
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
