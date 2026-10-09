#ifndef LAKOT_SERVER_INVENTORY_H
#define LAKOT_SERVER_INVENTORY_H

#include <cstdint>
#include <optional>
#include <vector>

#include "ItemTemplate.h"

namespace lakot
{

class ItemCatalog;

enum class ItemLocationType : uint32_t
{
    eInventory = 0,
    eEquipped = 1
};

// One actual item somebody owns. itemId is the database row - 0 means the
// instance has been created in memory but not yet persisted.
struct ItemInstance
{
    uint64_t itemId = 0;
    uint32_t templateId = 0;
    uint32_t count = 1;

    ItemLocationType location = ItemLocationType::eInventory;

    // For eInventory this is a BAG CELL index (see Inventory's grid
    // constants), naming the item's top-left cell; the item then covers its
    // template's width x height from there. For eEquipped it is the
    // EquipSlotType value instead.
    uint32_t slot = 0;
};

// A character's items: a spatial bag plus what is worn.
//
// The bag is a grid, not a list of interchangeable slots: every item occupies
// a rectangle of cells given by its template, so "is there room" is a packing
// question. A bag with eight cells free can still refuse a sword if none of
// those cells form a 1x3 column.
//
// NOT thread safe, deliberately. It is owned by a Character, which is owned by
// a Zone, and is only ever touched on that zone's thread - the same discipline
// that let WorldRegistry drop its mutex, and what will let Lua quest code
// manipulate items directly with no locking.
class Inventory
{
public:
    // The grid. Pages exist because a single 5x24 column would be unusable;
    // items never straddle a page boundary, so each page packs independently.
    // Raising kPageCount is the intended way to grow the bag later - slot
    // indices already in the database keep their meaning, because a slot is
    // numbered from the start of the whole grid and pages are appended.
    // Changing kGridWidth would renumber every existing slot and needs a
    // migration, which is why growth is expected in pages.
    static constexpr uint32_t kGridWidth = 5;
    static constexpr uint32_t kPageHeight = 12;
    static constexpr uint32_t kPageCount = 2;

    static constexpr uint32_t kCellsPerPage = kGridWidth * kPageHeight;
    static constexpr uint32_t kBagSlotCount = kCellsPerPage * kPageCount;

    // Where a bag slot sits, unpacked.
    struct CellPosition
    {
        uint32_t page = 0;
        uint32_t x = 0;
        uint32_t y = 0;
    };

    static CellPosition toCell(uint32_t pSlot);
    static uint32_t toSlot(uint32_t pPage, uint32_t pX, uint32_t pY);

    explicit Inventory(const ItemCatalog& pCatalog);

    void setItems(std::vector<ItemInstance> pItems);
    const std::vector<ItemInstance>& getItems() const;

    // Adds pCount of pTemplateId, filling existing stacks first and then
    // looking for a free rectangle for whatever is left. Returns how many
    // could NOT be placed (0 on full success), so a caller handing out a
    // quest reward can tell the player their bag is full instead of quietly
    // destroying the reward.
    uint32_t addItem(uint32_t pTemplateId, uint32_t pCount);

    // Removes up to pCount across every stack of pTemplateId. Returns how many
    // were actually removed.
    uint32_t removeItem(uint32_t pTemplateId, uint32_t pCount);

    uint32_t countItem(uint32_t pTemplateId) const;

    // Moves the item whose top-left cell is pBagSlot into its body slot,
    // swapping out whatever was there. Fails if the cell holds no item, the
    // item is not equippable, the character is too low level, or - now that
    // items have a footprint - the swapped-out item has nowhere to land.
    bool equip(uint32_t pBagSlot, uint32_t pCharacterLevel);
    bool unequip(EquipSlotType pSlot, std::optional<uint32_t> pBagSlot = std::nullopt);

    bool moveItem(uint32_t pFromSlot, uint32_t pToSlot);

    const ItemInstance* getEquipped(EquipSlotType pSlot) const;

    struct EquipmentBonuses
    {
        int32_t attack = 0;
        int32_t defense = 0;
        int32_t maxHealth = 0;
        int32_t maxMana = 0;
    };

    EquipmentBonuses getEquipmentBonuses() const;

private:
    const ItemCatalog& mCatalog;
    std::vector<ItemInstance> mItems;

    // One entry per bag cell, true where something sits. Rebuilt on demand
    // rather than maintained incrementally: a bag is 120 cells and a few
    // dozen items, so rebuilding costs nothing measurable, while an
    // incrementally-maintained map is one missed update away from letting two
    // items overlap.
    using Occupancy = std::vector<bool>;

    // pIgnoredIndex lets a caller ask "where could this go if the item it is
    // replacing were not there" - needed by equip(), which has to find a home
    // for the item coming OFF using the cells the item going ON is vacating.
    Occupancy buildOccupancy(size_t pIgnoredIndex = static_cast<size_t>(-1)) const;

    bool canPlaceAt(uint32_t pSlot, uint32_t pWidth, uint32_t pHeight, const Occupancy& pOccupancy) const;
    std::optional<uint32_t> findFreePlacement(uint32_t pWidth, uint32_t pHeight, const Occupancy& pOccupancy) const;

    void markOccupied(Occupancy& pOccupancy, uint32_t pSlot, uint32_t pWidth, uint32_t pHeight) const;

    // Footprint of an item instance, or 1x1 if its template is unknown - an
    // unknown template must still occupy something, or it would silently
    // overlap whatever is placed on top of it.
    void getFootprint(const ItemInstance& pItem, uint32_t& pWidth, uint32_t& pHeight) const;

    std::optional<size_t> findIndexAt(ItemLocationType pLocation, uint32_t pSlot) const;
};

}

#endif
