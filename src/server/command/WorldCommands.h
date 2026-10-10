#ifndef LAKOT_SERVER_WORLDCOMMANDS_H
#define LAKOT_SERVER_WORLDCOMMANDS_H

#include <memory>

#include "CommandContext.h"
#include "CommandReply.h"
#include "CommandTargetResolver.h"

#include "../network/controller/CommandController.h"

namespace lakot
{

class ItemCatalog;
class MonsterCatalog;
class WorldController;

class WorldCommands
{
public:
    WorldCommands(WorldController& pWorldController, const ItemCatalog& pItemCatalog, const MonsterCatalog& pMonsterCatalog);

    void registerCommands(CommandController& pCommandController);

private:
    using Method = void (WorldCommands::*)(const CommandContext&, std::shared_ptr<CommandReply>) const;

    WorldController& mWorldController;
    const ItemCatalog& mItemCatalog;
    const MonsterCatalog& mMonsterCatalog;
    CommandTargetResolver mResolver;

    CommandController::Handler bind(Method pMethod) const;

    void executeTeleport(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const;
    void executeTeleportToPlayer(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const;
    void executeSummon(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const;
    void executeItem(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const;
    void executeGold(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const;
    void executeLevel(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const;
    void executeHeal(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const;
    void executeSetHealth(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const;
    void executeSetMana(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const;
    void executeKill(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const;
    void executeRevive(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const;
    void executeSpawn(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const;
};

}

#endif
