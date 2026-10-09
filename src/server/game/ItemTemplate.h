#ifndef LAKOT_SERVER_ITEMTEMPLATE_H
#define LAKOT_SERVER_ITEMTEMPLATE_H

#include <cstdint>
#include <string>

namespace lakot
{

enum class ItemType : uint32_t
{
    eUndefined = 0,
    eEquipment = 1,
    eConsumable = 2,
    eMaterial = 3,
    eQuest = 4
};

// Which body slot an equippable item occupies. eNone means the item is not
// equippable at all, which is the only correct value for every non-eEquipment
// type.
enum class EquipSlotType : uint32_t
{
    eNone = 0,
    eWeapon = 1,
    eArmor = 2,
    eHelmet = 3,
    eBoots = 4,

    // Not a slot - the number of real slots, used to size the equipment
    // array. Keep last.
    eCount = 5
};

// The static definition of a kind of item: what every copy of it has in
// common. Loaded once at startup into ItemCatalog and never modified after,
// which is what lets every zone thread read it without a lock.
//
// Deliberately plain data with no behaviour. Anything an item DOES (a
// consumable's effect, a quest item's trigger) belongs in script once the Lua
// layer exists - a template is what the item IS, not what it does.
struct ItemTemplate
{
    uint32_t templateId = 0;
    std::string name;

    ItemType type = ItemType::eUndefined;
    EquipSlotType equipSlot = EquipSlotType::eNone;

    // 1 for anything that does not stack. Equipment is always 1 - two swords
    // are two rows, because each will eventually carry its own per-instance
    // state (enchant level, sockets, durability).
    uint32_t maxStack = 1;

    uint32_t requiredLevel = 1;

    // Footprint in bag cells. A sword is 1 wide and 3 tall, armour 2x2, a
    // potion 1x1 - so what fits is a packing question, not a count, and a
    // full bag can still have 8 free cells that no sword will go into.
    //
    // Both default to 1 so a template that predates this (or a content author
    // who omits it) gets the smallest sane footprint rather than a zero-sized
    // item that would occupy nothing and stack invisibly.
    uint32_t width = 1;
    uint32_t height = 1;

    // Icon file name, relative to the client's ui/textures/items directory.
    // A bare file name, not a path: the server has no business knowing where
    // a client keeps its art, only which picture this item is.
    std::string icon;
    std::string model;
    uint32_t tint = 0xFFFFFF;

    // Flat bonuses applied while the item is equipped. Percentage/conditional
    // modifiers are deliberately absent: those are the kind of rule that wants
    // real logic, and adding a second, weaker expression language here would
    // be exactly the mistake the scripting discussion warned about.
    int32_t bonusAttack = 0;
    int32_t bonusDefense = 0;
    int32_t bonusMaxHealth = 0;
    int32_t bonusMaxMana = 0;

    bool isEquippable() const
    {
        return type == ItemType::eEquipment && equipSlot != EquipSlotType::eNone;
    }

    bool isStackable() const
    {
        return maxStack > 1;
    }
};

}

#endif
