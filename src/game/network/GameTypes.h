#ifndef LAKOT_GAMETYPES_H
#define LAKOT_GAMETYPES_H

#include <cstdint>
#include <string>

namespace lakot
{

// The client-side counterparts of services/game.proto and the item-template
// half of services/inventory.proto.
//
// They live in one header rather than nested inside whichever controller
// happened to need them first: the enter-world handshake and the inventory
// pushes carry the same character, and two definitions of "the player's
// stats" would inevitably drift.

// A character's numbers, exactly as the server computed them. The derived
// values are received, never recalculated here - the balance formulas are the
// server's and a second copy would drift from them.
struct CharacterStats
{
    uint32_t level = 1;
    uint64_t experience = 0;
    uint64_t experienceForNextLevel = 0;

    int32_t health = 0;
    int32_t maxHealth = 0;
    int32_t mana = 0;
    int32_t maxMana = 0;

    int32_t strength = 0;
    int32_t dexterity = 0;
    int32_t intelligence = 0;
    int32_t vitality = 0;

    int32_t attack = 0;
    int32_t defense = 0;

    uint64_t gold = 0;
};

// Mirrors the server's ItemLocationType and services.game.ItemLocation.
enum class ItemLocationType : uint32_t
{
    eInventory = 0,
    eEquipped = 1
};

// Mirrors the server's EquipSlotType. Only meaningful for equippable items.
enum class EquipSlotType : uint32_t
{
    eNone = 0,
    eWeapon = 1,
    eArmor = 2,
    eHelmet = 3,
    eBoots = 4,
    eCount = 5
};

// Everything that decides how an item looks; the upgrade level will change it too.
struct ItemVisual
{
    uint32_t templateId = 0;
    uint32_t upgradeLevel = 0;

    bool operator==(const ItemVisual&) const = default;
};

struct EquippedVisual
{
    EquipSlotType slot = EquipSlotType::eNone;
    ItemVisual item;

    bool operator==(const EquippedVisual&) const = default;
};

// One item the player owns. Carries only the template id - what the item IS
// comes from the catalog, which is sent once per session.
struct OwnedItem
{
    uint32_t templateId = 0;
    uint32_t count = 0;
    ItemLocationType location = ItemLocationType::eInventory;
    uint32_t slot = 0;
};

// One entry of the item catalog, as received from the server.
struct ItemTemplate
{
    uint32_t templateId = 0;
    std::string name;

    uint32_t type = 0;
    EquipSlotType equipSlot = EquipSlotType::eNone;

    uint32_t maxStack = 1;
    uint32_t requiredLevel = 1;

    int32_t bonusAttack = 0;
    int32_t bonusDefense = 0;
    int32_t bonusMaxHealth = 0;
    int32_t bonusMaxMana = 0;

    // Footprint in bag cells - how many columns and rows this item covers.
    uint32_t width = 1;
    uint32_t height = 1;

    // Icon file name, resolved against ui/textures/items/. Empty when the
    // item has no art yet.
    std::string icon;

    std::string model;
    uint32_t tint = 0xFFFFFF;
};

// The bag's shape, as the server reports it. Held rather than hardcoded so
// growing the bag is a server change alone.
struct InventoryGrid
{
    uint32_t width = 5;
    uint32_t pageHeight = 12;
    uint32_t pageCount = 2;

    uint32_t getCellsPerPage() const { return width * pageHeight; }
    uint32_t getSlotCount() const { return getCellsPerPage() * pageCount; }
};

}

#endif
