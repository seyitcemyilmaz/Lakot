#ifndef LAKOT_SERVER_GMLOGREPOSITORY_H
#define LAKOT_SERVER_GMLOGREPOSITORY_H

#include <cstdint>
#include <string>

#include "BaseRepository.h"

namespace lakot
{

class GmLogRepository : public BaseRepository
{
public:
    struct Entry
    {
        uint64_t accountId = 0;
        uint64_t characterId = 0;
        std::string characterName;
        uint32_t gmLevel = 0;
        std::string command;
        std::string arguments;
        std::string target;
        std::string result;
    };

    explicit GmLogRepository(DatabaseManager& pDatabaseManager);

    void initializeTable() override;

    void record(Entry pEntry);
};

}

#endif
