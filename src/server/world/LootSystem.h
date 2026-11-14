#ifndef LAKOT_SERVER_LOOTSYSTEM_H
#define LAKOT_SERVER_LOOTSYSTEM_H

#include <cstdint>
#include <random>
#include <unordered_map>
#include <vector>

#include <connection.pb.h>

#include "ItemRules.h"
#include "MapData.h"
#include "MonsterCatalog.h"
#include "WorldRegistry.h"

#include "CombatSystem.h"

#include "../game/Character.h"
#include "../game/ItemCatalog.h"

namespace lakot
{

struct GroundItem
{
    uint64_t id = 0;
    uint32_t templateId = 0;
    uint32_t count = 0;
    uint64_t gold = 0;
    uint64_t ownerId = 0;
    double ownerUntil = 0.0;
    double despawnAt = 0.0;
};

class LootSystem
{
public:
    static constexpr double kOwnerSeconds = 10.0;
    static constexpr double kDespawnSeconds = 60.0;
    static constexpr float kScatterRadius = 0.6f;
    static constexpr size_t kMaxGroundItems = 500;

    LootSystem(const MapData& pMap, const MonsterCatalog& pMonsterCatalog, const ItemCatalog& pItemCatalog,
               WorldRegistry& pRegistry, uint64_t& pNextEntityId);

    void drop(const CombatReward& pReward, double pNow);
    void update(double pNow);

    bool dropItem(uint32_t pTemplateId, uint32_t pCount, float pX, float pZ, double pNow);
    bool hasGroundCapacity() const;

    const GroundItem* find(uint64_t pEntityId) const;

    services::world::PickupResult pickUp(Character& pPicker, const PlayerState& pPickerState, uint64_t pEntityId, double pNow);

private:
    const MapData& mMap;
    const MonsterCatalog& mMonsterCatalog;
    const ItemCatalog& mItemCatalog;
    WorldRegistry& mRegistry;
    uint64_t& mNextEntityId;

    std::mt19937 mRandom{std::random_device{}()};
    std::unordered_map<uint64_t, GroundItem> mItems;

    void scatter(float pCenterX, float pCenterZ, float& pX, float& pZ);
    void spawn(GroundItem pItem, float pX, float pZ);
    void remove(uint64_t pEntityId);
};

}

#endif
