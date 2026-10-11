#include "CommandTargetResolver.h"

#include "../network/controller/WorldController.h"

using namespace lakot;

CommandTargetResolver::CommandTargetResolver(WorldController& pWorldController)
    : mWorldController(pWorldController)
{

}

std::optional<CommandTarget> CommandTargetResolver::resolve(const CommandContext& pContext, size_t pArgumentIndex,
                                                            CommandReply& pReply) const
{
    CommandTarget tTarget;

    if (pContext.getArgumentCount() <= pArgumentIndex)
    {
        tTarget.characterId = pContext.characterId;
        tTarget.name = pContext.characterName;
        tTarget.gmLevel = pContext.gmLevel;
        pReply.setTarget(tTarget.name);
        return tTarget;
    }

    tTarget.name = std::string(pContext.getArgument(pArgumentIndex));
    pReply.setTarget(tTarget.name);

    auto tCharacterId = mWorldController.findOnlineCharacterIdByName(tTarget.name);
    auto tAccess = tCharacterId ? mWorldController.getCharacterAccess(*tCharacterId) : std::nullopt;

    if (!tAccess)
    {
        pReply.fail("Player is not online.");
        return std::nullopt;
    }

    tTarget.characterId = *tCharacterId;
    tTarget.gmLevel = tAccess->gmLevel;

    if (tTarget.characterId != pContext.characterId && tTarget.gmLevel >= pContext.gmLevel)
    {
        pReply.fail("You cannot target a player of your level or higher.");
        return std::nullopt;
    }

    return tTarget;
}
