#ifndef LAKOT_SERVER_DATABASECONFIG_H
#define LAKOT_SERVER_DATABASECONFIG_H

#include <cstdint>
#include <cstdlib>
#include <string>

namespace lakot
{

namespace
{
    inline std::string environmentOr(const char* pName, const std::string& pFallback)
    {
        const char* tValue = std::getenv(pName);
        return (tValue && *tValue) ? std::string(tValue) : pFallback;
    }
}

struct DatabaseConfig
{
    std::string host = environmentOr("LAKOT_DB_HOST", "localhost");
    uint16_t port = static_cast<uint16_t>(std::stoi(environmentOr("LAKOT_DB_PORT", "5432")));
    std::string user = environmentOr("LAKOT_DB_USER", "postgres");
    std::string password = environmentOr("LAKOT_DB_PASSWORD", "");
    std::string name = environmentOr("LAKOT_DB_NAME", "lakot_db");

    // 0 = auto-detect from hardware at startup (see DatabaseManager::initialize).
    unsigned int poolSize = 0;

    std::string toString() const
    {
        return "postgresql://" + user + ":" + password + "@" + host + ":" + std::to_string(port) + "/" + name;
    }

    std::string toSystemString() const
    {
        return "postgresql://" + user + ":" + password + "@" + host + ":" + std::to_string(port) + "/postgres";
    }
};

}

#endif
