#ifndef LAKOT_SERVER_COMMANDTARGETRESOLVER_H
#define LAKOT_SERVER_COMMANDTARGETRESOLVER_H

#include <cstdint>
#include <optional>
#include <string>

#include "CommandContext.h"
#include "CommandReply.h"

namespace lakot
{

class WorldController;

struct CommandTarget
{
    uint64_t characterId = 0;
    std::string name;
    uint32_t gmLevel = 0;
};

class CommandTargetResolver
{
public:
    explicit CommandTargetResolver(WorldController& pWorldController);

    std::optional<CommandTarget> resolve(const CommandContext& pContext, size_t pArgumentIndex,
                                         CommandReply& pReply) const;

private:
    WorldController& mWorldController;
};

}

#endif
