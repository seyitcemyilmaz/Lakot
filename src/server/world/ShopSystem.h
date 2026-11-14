#ifndef LAKOT_SERVER_SHOPSYSTEM_H
#define LAKOT_SERVER_SHOPSYSTEM_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <connection.pb.h>

#include "ItemRules.h"
#include "MapCatalog.h"
#include "WorldRegistry.h"

#include "Npc.h"

#include "../game/Character.h"
#include "../game/ItemCatalog.h"

namespace lakot
{

struct ShopOpenResult
{
    services::shop::ShopStatus status = services::shop::SHOP_STATUS_INVALID;
    std::string title;
    std::vector<uint32_t> templateIds;
};

class ShopSystem
{
public:
    static constexpr size_t kMaxShopItems = 60;

    ShopSystem(const ItemCatalog& pItemCatalog, const WorldRegistry& pRegistry, const std::unordered_map<uint64_t, Npc>& pNpcs);

    static bool validateStock(const MapCatalog& pMapCatalog, const ItemCatalog& pItemCatalog, std::string& pError);

    ShopOpenResult open(uint64_t pCharacterId, const Character& pCharacter, const PlayerState& pState, uint64_t pNpcId, double pNow);

    services::shop::ShopStatus buy(uint64_t pCharacterId, Character& pCharacter, const PlayerState& pState,
                                   uint64_t pNpcId, uint32_t pTemplateId, uint32_t pQuantity, double pNow);

    services::shop::ShopStatus sell(uint64_t pCharacterId, Character& pCharacter, const PlayerState& pState,
                                    uint64_t pNpcId, uint32_t pBagSlot, uint32_t pTemplateId, uint32_t pCount, double pNow,
                                    uint64_t& pGained);

    void forget(uint64_t pCharacterId);

private:
    const ItemCatalog& mItemCatalog;
    const WorldRegistry& mRegistry;
    const std::unordered_map<uint64_t, Npc>& mNpcs;

    std::unordered_map<uint64_t, double> mLastActionAt;

    services::shop::ShopStatus checkAccess(uint64_t pCharacterId, const Character& pCharacter, const PlayerState& pState,
                                           uint64_t pNpcId, double pNow, const Npc*& pNpc);

    bool isPaced(uint64_t pCharacterId, double pNow);
};

}

#endif
