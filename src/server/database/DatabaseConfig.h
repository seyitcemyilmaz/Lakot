#ifndef LAKOT_SERVER_DATABASECONFIG_H
#define LAKOT_SERVER_DATABASECONFIG_H

#include <string>

namespace lakot
{

struct DatabaseConfig
{
    std::string host = "localhost";
    uint16_t port = 5432;
    std::string user = "postgres";
    std::string password = "1";
    std::string name = "lakot_db";

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
