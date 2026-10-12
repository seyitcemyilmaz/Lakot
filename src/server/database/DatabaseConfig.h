#ifndef LAKOT_SERVER_DATABASECONFIG_H
#define LAKOT_SERVER_DATABASECONFIG_H

#include <charconv>
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

    inline uint16_t parsePort(const std::string& pText)
    {
        unsigned int tPort = 0;
        const char* tBegin = pText.data();
        const char* tEnd = tBegin + pText.size();
        const auto [tParsedEnd, tError] = std::from_chars(tBegin, tEnd, tPort);

        if (tError != std::errc{} || tParsedEnd != tEnd || tPort < 1 || tPort > 65535)
        {
            return 0;
        }

        return static_cast<uint16_t>(tPort);
    }

    inline std::string quoteConninfoValue(const std::string& pValue)
    {
        std::string tQuoted = "'";

        for (const char tChar : pValue)
        {
            if (tChar == '\'' || tChar == '\\')
            {
                tQuoted += '\\';
            }

            tQuoted += tChar;
        }

        tQuoted += '\'';
        return tQuoted;
    }
}

struct DatabaseConfig
{
    std::string host = environmentOr("LAKOT_DB_HOST", "localhost");
    uint16_t port = parsePort(environmentOr("LAKOT_DB_PORT", "5432"));
    std::string user = environmentOr("LAKOT_DB_USER", "postgres");
    std::string password = environmentOr("LAKOT_DB_PASSWORD", "");
    std::string name = environmentOr("LAKOT_DB_NAME", "lakot_db");

    // 0 = auto-detect from hardware at startup (see DatabaseManager::initialize).
    unsigned int poolSize = 0;

    bool isPortValid() const
    {
        return port != 0;
    }

    std::string toString() const
    {
        return toConnectionString(name);
    }

    std::string toSystemString() const
    {
        return toConnectionString("postgres");
    }

private:
    std::string toConnectionString(const std::string& pDatabase) const
    {
        return "host=" + quoteConninfoValue(host) +
               " port=" + std::to_string(port) +
               " dbname=" + quoteConninfoValue(pDatabase) +
               " user=" + quoteConninfoValue(user) +
               " password=" + quoteConninfoValue(password);
    }
};

}

#endif
