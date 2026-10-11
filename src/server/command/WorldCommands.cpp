#include "WorldCommands.h"

#include <limits>
#include <string>

#include "MonsterCatalog.h"

#include "../game/ItemCatalog.h"
#include "../network/controller/WorldController.h"

using namespace lakot;

namespace
{
    constexpr double kMaxCoordinate = 100000.0;
    constexpr int64_t kMaxGoldChange = 1000000000;
    constexpr int64_t kMaxCharacterLevel = 100;
    constexpr int64_t kMaxItemCount = 1000;
    constexpr int64_t kMaxSpawnCount = 10;
    constexpr int64_t kMaxUint32 = std::numeric_limits<uint32_t>::max();
    constexpr int64_t kMaxInt32 = std::numeric_limits<int32_t>::max();

    std::function<void(bool, const std::string&)> makeFinisher(std::shared_ptr<CommandReply> pReply)
    {
        return [pReply](bool pIsSuccess, const std::string& pText)
        {
            pReply->complete(pIsSuccess, pText);
        };
    }

    void replyUsage(CommandReply& pReply, const char* pUsage)
    {
        pReply.usage(std::string("Usage: ") + pUsage);
    }
}

WorldCommands::WorldCommands(WorldController& pWorldController, const ItemCatalog& pItemCatalog, const MonsterCatalog& pMonsterCatalog)
    : mWorldController(pWorldController)
    , mItemCatalog(pItemCatalog)
    , mMonsterCatalog(pMonsterCatalog)
    , mResolver(pWorldController)
{

}

CommandController::Handler WorldCommands::bind(Method pMethod) const
{
    return [this, pMethod](const CommandContext& pContext, std::shared_ptr<CommandReply> pReply)
    {
        (this->*pMethod)(pContext, std::move(pReply));
    };
}

void WorldCommands::registerCommands(CommandController& pCommandController)
{
    pCommandController.registerCommand({"tp", {"teleport"}, 1, 2, 2, "/tp <x> <z>", "Teleport to coordinates on this map.", bind(&WorldCommands::executeTeleport)});
    pCommandController.registerCommand({"tpto", {}, 1, 1, 1, "/tpto <player>", "Teleport to a player.", bind(&WorldCommands::executeTeleportToPlayer)});
    pCommandController.registerCommand({"summon", {}, 1, 1, 1, "/summon <player>", "Bring a player to you.", bind(&WorldCommands::executeSummon)});

    pCommandController.registerCommand({"item", {}, 2, 1, 3, "/item <id> [count] [player]", "Give items to a player.", bind(&WorldCommands::executeItem)});
    pCommandController.registerCommand({"gold", {}, 2, 1, 2, "/gold <amount> [player]", "Add or remove gold.", bind(&WorldCommands::executeGold)});
    pCommandController.registerCommand({"level", {}, 2, 1, 2, "/level <1-100> [player]", "Set a player's level.", bind(&WorldCommands::executeLevel)});
    pCommandController.registerCommand({"heal", {}, 2, 0, 1, "/heal [player]", "Restore health and mana.", bind(&WorldCommands::executeHeal)});
    pCommandController.registerCommand({"sethp", {}, 2, 1, 2, "/sethp <hp> [player]", "Set current health.", bind(&WorldCommands::executeSetHealth)});
    pCommandController.registerCommand({"setmp", {}, 2, 1, 2, "/setmp <mp> [player]", "Set current mana.", bind(&WorldCommands::executeSetMana)});
    pCommandController.registerCommand({"kill", {}, 2, 0, 1, "/kill [player]", "Kill a player.", bind(&WorldCommands::executeKill)});
    pCommandController.registerCommand({"revive", {}, 2, 0, 1, "/revive [player]", "Revive a dead player.", bind(&WorldCommands::executeRevive)});
    pCommandController.registerCommand({"spawn", {}, 2, 1, 2, "/spawn <monster id> [count]", "Spawn monsters next to you (1-10).", bind(&WorldCommands::executeSpawn)});
}

void WorldCommands::executeTeleport(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const
{
    auto tX = pContext.getNumber(0, -kMaxCoordinate, kMaxCoordinate);
    auto tZ = pContext.getNumber(1, -kMaxCoordinate, kMaxCoordinate);

    if (!tX || !tZ)
    {
        replyUsage(*pReply, "/tp <x> <z>");
        return;
    }

    pReply->setTarget(pContext.characterName);

    Zone* tZone = mWorldController.findCharacterZone(pContext.characterId);

    if (!tZone)
    {
        pReply->fail("You are not in the world.");
        return;
    }

    tZone->teleport(pContext.characterId, tZone->getMapId(), static_cast<float>(*tX), static_cast<float>(*tZ), makeFinisher(pReply));
}

void WorldCommands::executeTeleportToPlayer(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const
{
    auto tTarget = mResolver.resolve(pContext, 0, *pReply);

    if (!tTarget)
    {
        return;
    }

    Zone* tTargetZone = mWorldController.findCharacterZone(tTarget->characterId);

    if (!tTargetZone)
    {
        pReply->fail("Player is not online.");
        return;
    }

    tTargetZone->queryPosition(tTarget->characterId,
    [this, pReply, tActorId = pContext.characterId, tName = tTarget->name](bool pIsFound, uint32_t pMapId, float pX, float pZ)
    {
        Zone* tActorZone = pIsFound ? mWorldController.findCharacterZone(tActorId) : nullptr;

        if (!tActorZone)
        {
            pReply->fail("Player is not online.");
            return;
        }

        tActorZone->teleport(tActorId, pMapId, pX, pZ,
        [pReply, tName](bool pIsSuccess, const std::string& pText)
        {
            pReply->complete(pIsSuccess, pIsSuccess ? "Teleported to " + tName + "." : pText);
        });
    });
}

void WorldCommands::executeSummon(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const
{
    auto tTarget = mResolver.resolve(pContext, 0, *pReply);

    if (!tTarget)
    {
        return;
    }

    Zone* tActorZone = mWorldController.findCharacterZone(pContext.characterId);

    if (!tActorZone)
    {
        pReply->fail("You are not in the world.");
        return;
    }

    tActorZone->queryPosition(pContext.characterId,
    [this, pReply, tTargetId = tTarget->characterId, tName = tTarget->name](bool pIsFound, uint32_t pMapId, float pX, float pZ)
    {
        Zone* tTargetZone = pIsFound ? mWorldController.findCharacterZone(tTargetId) : nullptr;

        if (!tTargetZone)
        {
            pReply->fail("Player is not online.");
            return;
        }

        tTargetZone->teleport(tTargetId, pMapId, pX, pZ,
        [pReply, tName](bool pIsSuccess, const std::string& pText)
        {
            pReply->complete(pIsSuccess, pIsSuccess ? "Summoned " + tName + "." : pText);
        });
    });
}

void WorldCommands::executeItem(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const
{
    auto tItemId = pContext.getInteger(0, 1, kMaxUint32);
    auto tCount = pContext.getArgumentCount() >= 2 ? pContext.getInteger(1, 1, kMaxItemCount) : std::optional<int64_t>(1);

    if (!tItemId || !tCount)
    {
        replyUsage(*pReply, "/item <id> [count] [player]");
        return;
    }

    const ItemTemplate* tTemplate = mItemCatalog.find(static_cast<uint32_t>(*tItemId));

    if (!tTemplate)
    {
        pReply->fail("Unknown item id.");
        return;
    }

    auto tTarget = mResolver.resolve(pContext, 2, *pReply);

    if (!tTarget)
    {
        return;
    }

    mWorldController.withCharacter(tTarget->characterId,
    [pReply, tTemplateId = tTemplate->templateId, tItemName = tTemplate->name,
     tAmount = static_cast<uint32_t>(*tCount), tName = tTarget->name](Character& pCharacter)
    {
        uint32_t tLeftOver = pCharacter.giveItem(tTemplateId, tAmount);
        uint32_t tGiven = tAmount - tLeftOver;

        if (tGiven == 0)
        {
            pReply->fail("Inventory is full.");
            return false;
        }

        std::string tText = "Gave " + std::to_string(tGiven) + " x " + tItemName + " to " + tName + ".";

        if (tLeftOver > 0)
        {
            tText += " " + std::to_string(tLeftOver) + " did not fit.";
        }

        pReply->ok(tText);
        return true;
    });
}

void WorldCommands::executeGold(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const
{
    auto tAmount = pContext.getInteger(0, -kMaxGoldChange, kMaxGoldChange);

    if (!tAmount)
    {
        replyUsage(*pReply, "/gold <amount> [player]");
        return;
    }

    auto tTarget = mResolver.resolve(pContext, 1, *pReply);

    if (!tTarget)
    {
        return;
    }

    mWorldController.withCharacter(tTarget->characterId,
    [pReply, tDelta = *tAmount, tName = tTarget->name](Character& pCharacter)
    {
        pCharacter.addGold(tDelta);

        pReply->ok(tName + " gold changed by " + std::to_string(tDelta) + ", now " + std::to_string(pCharacter.getStats().gold) + ".");
        return true;
    });
}

void WorldCommands::executeLevel(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const
{
    auto tLevel = pContext.getInteger(0, 1, kMaxCharacterLevel);

    if (!tLevel)
    {
        replyUsage(*pReply, "/level <1-100> [player]");
        return;
    }

    auto tTarget = mResolver.resolve(pContext, 1, *pReply);

    if (!tTarget)
    {
        return;
    }

    mWorldController.withCharacter(tTarget->characterId,
    [pReply, tValue = static_cast<uint32_t>(*tLevel), tName = tTarget->name](Character& pCharacter)
    {
        pCharacter.setLevel(tValue);

        pReply->ok("Set " + tName + " to level " + std::to_string(tValue) + ".");
        return true;
    });
}

void WorldCommands::executeHeal(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const
{
    auto tTarget = mResolver.resolve(pContext, 0, *pReply);

    if (!tTarget)
    {
        return;
    }

    mWorldController.withCharacter(tTarget->characterId,
    [pReply, tName = tTarget->name](Character& pCharacter)
    {
        if (pCharacter.isDead())
        {
            pReply->fail(tName + " is dead. Use /revive.");
            return false;
        }

        pCharacter.setHealth(pCharacter.getMaxHealth());
        pCharacter.setMana(pCharacter.getMaxMana());

        pReply->ok("Healed " + tName + ".");
        return true;
    });
}

void WorldCommands::executeSetHealth(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const
{
    auto tValue = pContext.getInteger(0, 1, kMaxInt32);

    if (!tValue)
    {
        replyUsage(*pReply, "/sethp <hp> [player]");
        return;
    }

    auto tTarget = mResolver.resolve(pContext, 1, *pReply);

    if (!tTarget)
    {
        return;
    }

    mWorldController.withCharacter(tTarget->characterId,
    [pReply, tHealth = static_cast<int32_t>(*tValue), tName = tTarget->name](Character& pCharacter)
    {
        if (pCharacter.isDead())
        {
            pReply->fail(tName + " is dead. Use /revive.");
            return false;
        }

        if (tHealth > pCharacter.getMaxHealth())
        {
            pReply->fail("Health must be between 1 and " + std::to_string(pCharacter.getMaxHealth()) + ".");
            return false;
        }

        pCharacter.setHealth(tHealth);

        pReply->ok("Set " + tName + " health to " + std::to_string(tHealth) + ".");
        return true;
    });
}

void WorldCommands::executeSetMana(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const
{
    auto tValue = pContext.getInteger(0, 0, kMaxInt32);

    if (!tValue)
    {
        replyUsage(*pReply, "/setmp <mp> [player]");
        return;
    }

    auto tTarget = mResolver.resolve(pContext, 1, *pReply);

    if (!tTarget)
    {
        return;
    }

    mWorldController.withCharacter(tTarget->characterId,
    [pReply, tMana = static_cast<int32_t>(*tValue), tName = tTarget->name](Character& pCharacter)
    {
        if (pCharacter.isDead())
        {
            pReply->fail(tName + " is dead. Use /revive.");
            return false;
        }

        if (tMana > pCharacter.getMaxMana())
        {
            pReply->fail("Mana must be between 0 and " + std::to_string(pCharacter.getMaxMana()) + ".");
            return false;
        }

        pCharacter.setMana(tMana);

        pReply->ok("Set " + tName + " mana to " + std::to_string(tMana) + ".");
        return true;
    });
}

void WorldCommands::executeKill(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const
{
    auto tTarget = mResolver.resolve(pContext, 0, *pReply);

    if (!tTarget)
    {
        return;
    }

    Zone* tZone = mWorldController.findCharacterZone(tTarget->characterId);

    if (!tZone)
    {
        pReply->fail("Player is not online.");
        return;
    }

    tZone->killPlayer(tTarget->characterId, makeFinisher(pReply));
}

void WorldCommands::executeRevive(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const
{
    auto tTarget = mResolver.resolve(pContext, 0, *pReply);

    if (!tTarget)
    {
        return;
    }

    Zone* tZone = mWorldController.findCharacterZone(tTarget->characterId);

    if (!tZone)
    {
        pReply->fail("Player is not online.");
        return;
    }

    tZone->revivePlayer(tTarget->characterId, makeFinisher(pReply));
}

void WorldCommands::executeSpawn(const CommandContext& pContext, std::shared_ptr<CommandReply> pReply) const
{
    auto tMonsterId = pContext.getInteger(0, 1, kMaxUint32);
    auto tCount = pContext.getArgumentCount() >= 2 ? pContext.getInteger(1, 1, kMaxSpawnCount) : std::optional<int64_t>(1);

    if (!tMonsterId || !tCount)
    {
        replyUsage(*pReply, "/spawn <monster id> [count]");
        return;
    }

    const MonsterTemplate* tTemplate = mMonsterCatalog.find(static_cast<uint32_t>(*tMonsterId));

    if (!tTemplate)
    {
        pReply->fail("Unknown monster id.");
        return;
    }

    pReply->setTarget(tTemplate->name);

    Zone* tZone = mWorldController.findCharacterZone(pContext.characterId);

    if (!tZone)
    {
        pReply->fail("You are not in the world.");
        return;
    }

    tZone->spawnMonstersNear(pContext.characterId, *tTemplate, static_cast<uint32_t>(*tCount), makeFinisher(pReply));
}
