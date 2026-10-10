#ifndef LAKOT_SERVER_CHARACTERACCESS_H
#define LAKOT_SERVER_CHARACTERACCESS_H

#include <chrono>
#include <cstdint>

namespace lakot
{

struct CharacterAccess
{
    uint64_t accountId = 0;
    uint32_t gmLevel = 0;
    int64_t mutedUntilEpoch = 0;

    int64_t getMuteRemainingSeconds() const
    {
        int64_t tNow = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        return mutedUntilEpoch > tNow ? mutedUntilEpoch - tNow : 0;
    }
};

}

#endif
