#ifndef LAKOT_SERVER_CHARACTER_H
#define LAKOT_SERVER_CHARACTER_H

#include <cstdint>
#include <optional>
#include <string>

#include "CharacterStats.h"
#include "Inventory.h"

namespace lakot
{

class ItemCatalog;

// Everything needed to reconstruct a character somewhere else: loading one
// from storage into a zone, and handing one from one zone to another through
// a portal, are the same operation with the same payload.
//
// A plain value, not a reference to a live Character, precisely because it
// crosses a thread boundary in both cases - the source zone is done with the
// character by the time the destination sees this.
struct CharacterSnapshot
{
    uint64_t characterId = 0;
    std::string name;
    uint32_t kingdom = 0;
    CharacterStats stats;
    std::vector<ItemInstance> items;
};

// One player's character while it exists in the world: identity, stats and
// items. Position is deliberately NOT here - that lives in WorldRegistry,
// which is the spatial index and needs to iterate positions densely without
// dragging inventories along.
//
// Owned by a Zone and touched only on that zone's thread, like everything
// else the zone owns. That is what will let Lua quest code call
// giveItem/addExperience directly, on the same thread, with no locking - the
// single most important property of this class for what comes next.
class Character
{
public:
    Character(uint64_t pCharacterId, std::string pName, const ItemCatalog& pCatalog);

    uint64_t getId() const;
    const std::string& getName() const;

    const CharacterStats& getStats() const;
    const Inventory& getInventory() const;

    // ---- Derived values. Never stored - recomputed from base stats plus
    // whatever is currently equipped, so a balance change is a code change
    // and nothing else. ----
    int32_t getMaxHealth() const;
    int32_t getMaxMana() const;
    int32_t getAttack() const;
    int32_t getDefense() const;

    uint32_t getKingdom() const;
    void setKingdom(uint32_t pKingdom);

    bool isDead() const;
    bool takeDamage(int32_t pAmount);
    void revive();

    // ---- Mutations. Each marks the character dirty, so the autosave only
    // writes characters that actually changed. ----

    void setStats(const CharacterStats& pStats);
    void setItems(std::vector<ItemInstance> pItems);

    // Returns how many could not fit (0 on full success) - the caller has to
    // decide what to do about a full bag, this will not silently destroy the
    // remainder.
    uint32_t giveItem(uint32_t pTemplateId, uint32_t pCount);
    uint32_t takeItem(uint32_t pTemplateId, uint32_t pCount);
    uint32_t countItem(uint32_t pTemplateId) const;

    bool equipItem(uint32_t pBagSlot);
    bool unequipItem(EquipSlotType pSlot, std::optional<uint32_t> pBagSlot = std::nullopt);
    bool moveItem(uint32_t pFromSlot, uint32_t pToSlot);

    // Adds experience and applies as many level-ups as it covers. Returns the
    // number of levels gained, so the caller can tell the client.
    uint32_t addExperience(uint64_t pAmount);

    void addGold(int64_t pAmount);

    // Current health/mana pulled back inside their (possibly just changed)
    // maxima. Called automatically after anything that moves a maximum.
    void refreshVitals();

    bool isDirty() const;
    void clearDirty();

private:
    uint64_t mCharacterId;
    std::string mName;

    uint32_t mKingdom{0};
    CharacterStats mStats;
    Inventory mInventory;

    bool mIsDirty{false};

    void markDirty();
};

}

#endif
