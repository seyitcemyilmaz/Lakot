#include "ModerationCommands.h"

#include "../database/repository/AccountRepository.h"
#include "../network/NetworkManager.h"
#include "../network/controller/WorldController.h"

using namespace lakot;

ModerationCommands::ModerationCommands(WorldController& pWorldController, NetworkManager& pNetworkManager, AccountRepository& pAccountRepository)
    : mWorldController(pWorldController)
    , mNetworkManager(pNetworkManager)
    , mAccountRepository(pAccountRepository)
    , mResolver(pWorldController)
{

}

CommandController::Handler ModerationCommands::bind(Method pMethod) const
{
    return [this, pMethod](const CommandContext& pContext, std::shared_ptr<CommandReply> pReply)
    {
        (this->*pMethod)(pContext, std::move(pReply));
    };
}

void ModerationCommands::registerCommands(CommandController& pCommandController)
{
    pCommandController.registerCommand({"kick", {}, 1, 1, 1, "/kick <player>", "Disconnect a player.", bind(&ModerationCommands::executeKick)});
    pCommandController.registerCommand({"mute", {}, 1, 2, 2, "/mute <player> <minutes>", "Mute a player for 0-10080 minutes (0 lifts it).", bind(&ModerationCommands::executeMute)});
    pCommandController.registerCommand({"announce", {}, 1, 1, CommandContext::kMaxArguments, "/announce <message>", "Send a message to every player.", bind(&ModerationCommands::executeAnnounce)});
}

void ModerationCommands::sendSystemMessage(uint64_t pCharacterId, services::chat::SystemMessageKind pKind, const std::string& pText) const
{
    auto tSession = mNetworkManager.getSessionRegistry().getByCharacter(pCharacterId);

    if (!tSession)
    {
        return;
    }

    connection::Message tMessage;
    auto* tSystem = tMessage.mutable_response()->mutable_system_message();
    tSystem->set_kind(pKind);
    tSystem->set_text(pText);

    tSession->send(tMessage);
}

void ModerationCommands::executeKick(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const
{
    auto tTarget = mResolver.resolve(pContext, 0, *pReply);

    if (!tTarget)
    {
        return;
    }

    auto tSession = mNetworkManager.getSessionRegistry().getByCharacter(tTarget->characterId);

    if (!tSession)
    {
        auto tAccess = mWorldController.getCharacterAccess(tTarget->characterId);

        if (!tAccess)
        {
            pReply->fail("Player is not online.");
            return;
        }

        mNetworkManager.endDetachedCharacter(tAccess->accountId, tTarget->characterId);
        pReply->ok("Kicked " + tTarget->name + ".");
        return;
    }

    sendSystemMessage(tTarget->characterId, services::chat::SYSTEM_MESSAGE_WARNING, "You have been disconnected by a game master.");

    mNetworkManager.endSession(tSession);
    tSession->closeAfter(kKickCloseDelay);

    pReply->ok("Kicked " + tTarget->name + ".");
}

void ModerationCommands::executeMute(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const
{
    auto tMinutes = pContext.getInteger(1, 0, kMaxMuteMinutes);

    if (!tMinutes)
    {
        pReply->usage("Usage: /mute <player> <minutes>");
        return;
    }

    std::string tName(pContext.getArgument(0));
    pReply->setTarget(tName);

    mAccountRepository.muteByCharacterName(tName, static_cast<uint32_t>(*tMinutes), pContext.gmLevel, pContext.accountId,
    [this, pReply, tName, tDuration = *tMinutes](const AccountRepository::MuteOutcome& pOutcome)
    {
        if (pOutcome.result == AccountRepository::MuteResultType::eNotFound)
        {
            pReply->fail("Player not found.");
            return;
        }

        if (pOutcome.result == AccountRepository::MuteResultType::eProtected)
        {
            pReply->fail("You cannot target a player of your level or higher.");
            return;
        }

        auto tOnlineId = mWorldController.findOnlineCharacterIdByAccount(pOutcome.accountId);

        if (tOnlineId)
        {
            mWorldController.setMutedUntil(*tOnlineId, pOutcome.mutedUntilEpoch);

            sendSystemMessage(*tOnlineId, services::chat::SYSTEM_MESSAGE_WARNING,
                              tDuration == 0 ? std::string("You have been unmuted.")
                                             : "You have been muted for " + std::to_string(tDuration) + " minute(s).");
        }

        pReply->ok(tDuration == 0 ? "Unmuted " + pOutcome.characterName + "."
                                  : "Muted " + pOutcome.characterName + " for " + std::to_string(tDuration) + " minute(s).");
    });
}

void ModerationCommands::executeAnnounce(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const
{
    connection::Message tMessage;
    auto* tSystem = tMessage.mutable_response()->mutable_system_message();
    tSystem->set_kind(services::chat::SYSTEM_MESSAGE_ANNOUNCE);
    tSystem->set_text(pContext.getRest(0));

    auto tPacket = NetworkSession<connection::Message>::makePacket(tMessage);
    auto& tRegistry = mNetworkManager.getSessionRegistry();
    size_t tRecipients = 0;

    for (uint64_t tCharacterId : tRegistry.getAllCharacterIds())
    {
        if (auto tSession = tRegistry.getByCharacter(tCharacterId))
        {
            tSession->send(tPacket);
            ++tRecipients;
        }
    }

    pReply->setTarget("all");
    pReply->ok("Announcement sent to " + std::to_string(tRecipients) + " player(s).");
}
