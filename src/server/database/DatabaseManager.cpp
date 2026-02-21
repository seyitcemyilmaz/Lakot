#include "DatabaseManager.h"

#include <iostream>

using namespace lakot;

DatabaseManager::~DatabaseManager()
{
    stop();
}

DatabaseManager::DatabaseManager()
    : mWorkGuard(boost::asio::make_work_guard(mDatabaseContext))
{
    mDatabaseThread = std::thread([this]()
    {
        runWorker();
    });
}

void DatabaseManager::initialize(const DatabaseConfig& pConfig)
{
    boost::asio::post(mDatabaseContext, [this, pConfig]()
    {
        try
        {
            std::cout << "[Database] Veritabani kontrol ediliyor..." << std::endl;
            ensureDatabaseExists(pConfig);

            mDatabaseConnection = std::make_unique<pqxx::connection>(pConfig.toString());

            if (mDatabaseConnection->is_open())
            {
                std::cout << "[Database] Baglanti BASARILI: " << mDatabaseConnection->dbname() << std::endl;
            }
        }
        catch (const std::exception& tException)
        {
            std::cerr << "[Database] Kritik Baslatma Hatasi: " << tException.what() << std::endl;
        }
    });
}

void DatabaseManager::executeAsync(DatabaseTask pTask)
{
    boost::asio::post(mDatabaseContext, [this, pTask]()
    {
        if (mDatabaseConnection && mDatabaseConnection->is_open())
        {
            try
            {
                pTask(*mDatabaseConnection);
            }
            catch (const std::exception& tException)
            {
                std::cerr << "[Database] Sorgu Hatasi: " << tException.what() << std::endl;
            }
        }
        else
        {
            std::cerr << "[Database] Hata: Baglanti yok, sorgu calistirilamadi." << std::endl;
        }
    });
}

void DatabaseManager::ensureDatabaseExists(const DatabaseConfig& pConfig)
{
    try
    {
        pqxx::connection tSysConn(pConfig.toSystemString());
        pqxx::nontransaction tNtx(tSysConn);

        std::string tCheckQuery = "SELECT 1 FROM pg_database WHERE datname = '" + tNtx.esc(pConfig.name) + "'";
        pqxx::result tRes = tNtx.exec(tCheckQuery);

        if (tRes.empty())
        {
            std::cout << "[Database] Olusturuluyor: " << pConfig.name << std::endl;
            tNtx.exec("CREATE DATABASE " + pConfig.name);
            std::cout << "[Database] Olusturuldu." << std::endl;
        }
    }
    catch (const std::exception& tException)
    {
        std::string tErr = tException.what();
        if (tErr.find("already exists") == std::string::npos)
        {
            std::cerr << "[Database] CreateDB Uyarisi: " << tException.what() << std::endl;
        }
    }
}

void DatabaseManager::stop()
{
    mDatabaseContext.stop();

    if (mDatabaseThread.joinable())
    {
        mDatabaseThread.join();
    }
}

void DatabaseManager::runWorker()
{
    mDatabaseContext.run();
}

bool DatabaseManager::isConnected() const
{
    return mDatabaseConnection && mDatabaseConnection->is_open();
}
