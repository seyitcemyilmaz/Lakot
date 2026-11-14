#include "ShopSystem.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <syncstream>

using namespace lakot;
using namespace services::shop;

ShopSystem::ShopSystem(const ItemCatalog& pItemCatalog, const WorldRegistry& pRegistry, const std::unordered_map<uint64_t, Npc>& pNpcs)
    : mItemCatalog(pItemCatalog)
    , mRegistry(pRegistry)
    , mNpcs(pNpcs)
{
}

bool ShopSystem::validateStock(const MapCatalog& pMapCatalog, const ItemCatalog& pItemCatalog, std::string& pError)
{
    for (uint32_t tMapId : pMapCatalog.getMapIds())
    {
        const MapData* tMap = pMapCatalog.getMap(tMapId);

        if (!tMap)
        {
            continue;
        }

        for (const MapNpc& tNpc : tMap->getNpcs())
        {
            std::string tWhere = "map " + std::to_string(tMapId) + " npc " + tNpc.name;

            if (tNpc.shopItems.size() > kMaxShopItems)
            {
                pError = tWhere + " sells more than " + std::to_string(kMaxShopItems) + " items";
                return false;
            }

            for (uint32_t tTemplateId : tNpc.shopItems)
            {
                const ItemTemplate* tTemplate = pItemCatalog.find(tTemplateId);

                if (!tTemplate)
                {
                    pError = tWhere + " sells unknown item " + std::to_string(tTemplateId);
                    return false;
                }

                if (tTemplate->price == 0 || tTemplate->type == ItemType::eQuest)
                {
                    pError = tWhere + " sells an item that cannot be bought: " + tTemplate->name;
                    return false;
                }
            }
        }
    }

    return true;
}

bool ShopSystem::isPaced(uint64_t pCharacterId, double pNow)
{
    auto [tLast, tIsFirst] = mLastActionAt.try_emplace(pCharacterId, pNow);

    if (tIsFirst)
    {
        return true;
    }

    if (pNow - tLast->second < ItemRules::kShopActionCooldownSeconds)
    {
        return false;
    }

    tLast->second = pNow;
    return true;
}

ShopStatus ShopSystem::checkAccess(uint64_t pCharacterId, const Character& pCharacter, const PlayerState& pState,
                                   uint64_t pNpcId, double pNow, const Npc*& pNpc)
{
    if (!isPaced(pCharacterId, pNow))
    {
        return SHOP_STATUS_TOO_FAST;
    }

    if (pCharacter.isDead())
    {
        return SHOP_STATUS_DEAD;
    }

    auto tNpc = mNpcs.find(pNpcId);
    const PlayerState* tNpcState = tNpc == mNpcs.end() ? nullptr : mRegistry.getState(pNpcId);

    if (!tNpcState || tNpc->second.shopItems.empty())
    {
        return SHOP_STATUS_INVALID;
    }

    float tDx = pState.x - tNpcState->x;
    float tDz = pState.z - tNpcState->z;

    if (!(tDx * tDx + tDz * tDz <= ItemRules::kShopRange * ItemRules::kShopRange))
    {
        return SHOP_STATUS_OUT_OF_RANGE;
    }

    pNpc = &tNpc->second;
    return SHOP_STATUS_OK;
}

ShopOpenResult ShopSystem::open(uint64_t pCharacterId, const Character& pCharacter, const PlayerState& pState, uint64_t pNpcId, double pNow)
{
    ShopOpenResult tResult;
    const Npc* tNpc = nullptr;
    tResult.status = checkAccess(pCharacterId, pCharacter, pState, pNpcId, pNow, tNpc);

    if (tResult.status == SHOP_STATUS_OK)
    {
        tResult.title = tNpc->name;
        tResult.templateIds = tNpc->shopItems;
    }

    return tResult;
}

ShopStatus ShopSystem::buy(uint64_t pCharacterId, Character& pCharacter, const PlayerState& pState,
                           uint64_t pNpcId, uint32_t pTemplateId, uint32_t pQuantity, double pNow)
{
    const Npc* tNpc = nullptr;
    ShopStatus tAccess = checkAccess(pCharacterId, pCharacter, pState, pNpcId, pNow, tNpc);

    if (tAccess != SHOP_STATUS_OK)
    {
        return tAccess;
    }

    if (std::find(tNpc->shopItems.begin(), tNpc->shopItems.end(), pTemplateId) == tNpc->shopItems.end())
    {
        return SHOP_STATUS_NOT_SOLD_HERE;
    }

    const ItemTemplate* tTemplate = mItemCatalog.find(pTemplateId);

    if (!tTemplate || tTemplate->price == 0 || pQuantity == 0 || pQuantity > tTemplate->maxStack)
    {
        return SHOP_STATUS_INVALID;
    }

    uint64_t tCost = static_cast<uint64_t>(tTemplate->price) * pQuantity;

    if (pCharacter.getStats().gold < tCost)
    {
        return SHOP_STATUS_NOT_ENOUGH_GOLD;
    }

    if (!pCharacter.canGiveItem(pTemplateId, pQuantity))
    {
        return SHOP_STATUS_BAG_FULL;
    }

    pCharacter.addGold(-static_cast<int64_t>(tCost));
    uint32_t tLeftOver = pCharacter.giveItem(pTemplateId, pQuantity);

    if (tLeftOver != 0)
    {
        std::osyncstream(std::cerr) << "[Shop] canGiveItem ile giveItem ayristi: karakter " << pCharacterId
                                    << " esya " << pTemplateId << " artan " << tLeftOver << std::endl;

        pCharacter.takeItem(pTemplateId, pQuantity - tLeftOver);
        pCharacter.addGold(static_cast<int64_t>(tCost));
        assert(tLeftOver == 0);
        return SHOP_STATUS_BAG_FULL;
    }

    return SHOP_STATUS_OK;
}

ShopStatus ShopSystem::sell(uint64_t pCharacterId, Character& pCharacter, const PlayerState& pState,
                            uint64_t pNpcId, uint32_t pBagSlot, uint32_t pTemplateId, uint32_t pCount, double pNow,
                            uint64_t& pGained)
{
    pGained = 0;

    const Npc* tNpc = nullptr;
    ShopStatus tAccess = checkAccess(pCharacterId, pCharacter, pState, pNpcId, pNow, tNpc);

    if (tAccess != SHOP_STATUS_OK)
    {
        return tAccess;
    }

    const ItemInstance* tStack = nullptr;

    for (const ItemInstance& tItem : pCharacter.getInventory().getItems())
    {
        if (tItem.location == ItemLocationType::eInventory && tItem.slot == pBagSlot && tItem.templateId == pTemplateId)
        {
            tStack = &tItem;
            break;
        }
    }

    const ItemTemplate* tTemplate = tStack ? mItemCatalog.find(pTemplateId) : nullptr;

    if (!tTemplate)
    {
        return SHOP_STATUS_INVALID;
    }

    // TODO: bound items must report NOT_SELLABLE here once item binding exists.
    if (tTemplate->sellPrice == 0)
    {
        return SHOP_STATUS_NOT_SELLABLE;
    }

    uint32_t tActualCount = (pCount == 0 || pCount > tStack->count) ? tStack->count : pCount;
    uint64_t tIncome = static_cast<uint64_t>(tTemplate->sellPrice) * tActualCount;

    if (pCharacter.getStats().gold > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) - tIncome)
    {
        return SHOP_STATUS_GOLD_LIMIT;
    }

    std::optional<ItemInstance> tRemoved = pCharacter.takeItemAt(pBagSlot, pTemplateId, tActualCount);

    if (!tRemoved)
    {
        return SHOP_STATUS_INVALID;
    }

    pCharacter.addGold(static_cast<int64_t>(tIncome));
    pGained = tIncome;
    return SHOP_STATUS_OK;
}

void ShopSystem::forget(uint64_t pCharacterId)
{
    mLastActionAt.erase(pCharacterId);
}
