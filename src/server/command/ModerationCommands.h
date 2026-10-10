#ifndef LAKOT_SERVER_MODERATIONCOMMANDS_H
#define LAKOT_SERVER_MODERATIONCOMMANDS_H

#include <chrono>
#include <memory>
#include <string>

#include "CommandContext.h"
#include "CommandReply.h"
#include "CommandTargetResolver.h"

#include "../network/controller/CommandController.h"

namespace lakot
{

class AccountRepository;
class NetworkManager;
class WorldController;

class ModerationCommands
{
public:
    ModerationCommands(WorldController& pWorldController, NetworkManager& pNetworkManager, AccountRepository& pAccountRepository);

    void registerCommands(CommandController& pCommandController);

private:
    using Method = void (ModerationCommands::*)(const CommandContext&, std::shared_ptr<CommandReply>) const;

    static constexpr int64_t kMaxMuteMinutes = 10080;
    static constexpr std::chrono::milliseconds kKickCloseDelay{300};

    WorldController& mWorldController;
    NetworkManager& mNetworkManager;
    AccountRepository& mAccountRepository;
    CommandTargetResolver mResolver;

    CommandController::Handler bind(Method pMethod) const;

    void sendSystemMessage(uint64_t pCharacterId, services::chat::SystemMessageKind pKind, const std::string& pText) const;

    void executeKick(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const;
    void executeMute(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const;
    void executeAnnounce(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const;
};

}

#endif
