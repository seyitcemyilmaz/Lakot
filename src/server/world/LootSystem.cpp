#include "LootSystem.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace lakot;

LootSystem::LootSystem(const MapData& pMap, const MonsterCatalog& pMonsterCatalog, const ItemCatalog& pItemCatalog,
                       WorldRegistry& pRegistry, uint64_t& pNextEntityId)
    : mMap(pMap)
    , mMonsterCatalog(pMonsterCatalog)
    , mItemCatalog(pItemCatalog)
    , mRegistry(pRegistry)
    , mNextEntityId(pNextEntityId)
{
}

void LootSystem::drop(const CombatReward& pReward, double pNow)
{
    const MonsterTemplate* tTemplate = mMonsterCatalog.find(pReward.monsterTemplateId);

    if (!tTemplate)
    {
        return;
    }

    std::uniform_real_distribution<float> tUnit(0.0f, 1.0f);

    auto tScatter = [&](float& pX, float& pZ)
    {
        float tAngle = tUnit(mRandom) * 6.2831853f;
        float tDistance = std::sqrt(tUnit(mRandom)) * kScatterRadius;
        float tX = pReward.x + std::cos(tAngle) * tDistance;
        float tZ = pReward.z + std::sin(tAngle) * tDistance;

        pX = mMap.isWalkable(tX, tZ) ? tX : pReward.x;
        pZ = mMap.isWalkable(tX, tZ) ? tZ : pReward.z;
    };

    // TODO: party loot - ownerId becomes a party id once parties exist
    GroundItem tBase;
    tBase.ownerId = pReward.characterId;
    tBase.ownerUntil = pNow + kOwnerSeconds;
    tBase.despawnAt = pNow + kDespawnSeconds;

    if (tTemplate->goldMax > 0 && tUnit(mRandom) < tTemplate->goldChance)
    {
        std::uniform_int_distribution<uint32_t> tAmount(tTemplate->goldMin, tTemplate->goldMax);
        GroundItem tGold = tBase;
        float tFactor = getMonsterLevelFactor(*tTemplate, pReward.level);
        tGold.gold = static_cast<uint64_t>(std::lround(tAmount(mRandom) * tFactor));

        if (tGold.gold > 0)
        {
            float tX = 0.0f;
            float tZ = 0.0f;
            tScatter(tX, tZ);
            spawn(tGold, tX, tZ);
        }
    }

    auto tSpawnDrop = [&](const MonsterDrop& pDrop)
    {
        std::uniform_int_distribution<uint32_t> tAmount(pDrop.minCount, pDrop.maxCount);
        GroundItem tItem = tBase;
        tItem.templateId = pDrop.itemId;
        tItem.count = tAmount(mRandom);

        float tX = 0.0f;
        float tZ = 0.0f;
        tScatter(tX, tZ);
        spawn(tItem, tX, tZ);
    };

    for (const MonsterDrop& tDrop : tTemplate->drops)
    {
        if (tDrop.guaranteed && mItemCatalog.find(tDrop.itemId))
        {
            tSpawnDrop(tDrop);
        }
    }

    for (uint32_t tRoll = 0; tRoll < tTemplate->lootRolls; ++tRoll)
    {
        for (const MonsterDrop& tDrop : tTemplate->drops)
        {
            if (!tDrop.guaranteed && tUnit(mRandom) < tDrop.chance && mItemCatalog.find(tDrop.itemId))
            {
                tSpawnDrop(tDrop);
            }
        }
    }
}

void LootSystem::update(double pNow)
{
    std::vector<uint64_t> tExpired;

    for (const auto& [tId, tItem] : mItems)
    {
        if (pNow >= tItem.despawnAt)
        {
            tExpired.push_back(tId);
        }
    }

    for (uint64_t tId : tExpired)
    {
        remove(tId);
    }
}

const GroundItem* LootSystem::find(uint64_t pEntityId) const
{
    auto tIterator = mItems.find(pEntityId);
    return tIterator == mItems.end() ? nullptr : &tIterator->second;
}

services::world::PickupResult LootSystem::pickUp(Character& pPicker, const PlayerState& pPickerState, uint64_t pEntityId, double pNow)
{
    auto tIterator = mItems.find(pEntityId);
    const PlayerState* tItemState = tIterator == mItems.end() ? nullptr : mRegistry.getState(pEntityId);

    if (!tItemState)
    {
        return services::world::PICKUP_RESULT_NOT_FOUND;
    }

    if (pPicker.isDead())
    {
        return services::world::PICKUP_RESULT_DEAD;
    }

    float tDx = tItemState->x - pPickerState.x;
    float tDz = tItemState->z - pPickerState.z;

    if (tDx * tDx + tDz * tDz > kPickupRange * kPickupRange)
    {
        return services::world::PICKUP_RESULT_TOO_FAR;
    }

    const GroundItem tItem = tIterator->second;

    if (tItem.ownerId != pPicker.getId() && pNow < tItem.ownerUntil)
    {
        return services::world::PICKUP_RESULT_NOT_OWNER;
    }

    float tX = tItemState->x;
    float tZ = tItemState->z;

    if (tItem.gold > 0)
    {
        pPicker.addGold(static_cast<int64_t>(tItem.gold));
        remove(pEntityId);
        return services::world::PICKUP_RESULT_OK;
    }

    uint32_t tLeftOver = pPicker.giveItem(tItem.templateId, tItem.count);

    if (tLeftOver == tItem.count)
    {
        return services::world::PICKUP_RESULT_INVENTORY_FULL;
    }

    remove(pEntityId);

    if (tLeftOver > 0)
    {
        GroundItem tRest = tItem;
        tRest.count = tLeftOver;
        spawn(tRest, tX, tZ);
    }

    return services::world::PICKUP_RESULT_OK;
}

void LootSystem::spawn(GroundItem pItem, float pX, float pZ)
{
    pItem.id = mNextEntityId++;

    PlayerState tState;
    tState.x = pX;
    tState.z = pZ;
    tState.y = mMap.getHeightAt(pX, pZ);

    std::string tName = "Gold";

    if (pItem.gold == 0)
    {
        const ItemTemplate* tTemplate = mItemCatalog.find(pItem.templateId);
        tName = tTemplate ? tTemplate->name : std::string();
    }

    mRegistry.add(pItem.id, std::move(tName), tState, false);
    mItems.emplace(pItem.id, pItem);
}

void LootSystem::remove(uint64_t pEntityId)
{
    mRegistry.remove(pEntityId);
    mItems.erase(pEntityId);
}
