#include "Inventory.h"

#include <algorithm>
#include <utility>

#include "ItemCatalog.h"

using namespace lakot;

Inventory::CellPosition Inventory::toCell(uint32_t pSlot)
{
    CellPosition tCell;
    tCell.page = pSlot / kCellsPerPage;

    uint32_t tWithinPage = pSlot % kCellsPerPage;
    tCell.y = tWithinPage / kGridWidth;
    tCell.x = tWithinPage % kGridWidth;

    return tCell;
}

uint32_t Inventory::toSlot(uint32_t pPage, uint32_t pX, uint32_t pY)
{
    return pPage * kCellsPerPage + pY * kGridWidth + pX;
}

Inventory::Inventory(const ItemCatalog& pCatalog)
    : mCatalog(pCatalog)
{

}

void Inventory::setItems(std::vector<ItemInstance> pItems)
{
    mItems = std::move(pItems);
}

const std::vector<ItemInstance>& Inventory::getItems() const
{
    return mItems;
}

void Inventory::getFootprint(const ItemInstance& pItem, uint32_t& pWidth, uint32_t& pHeight) const
{
    const ItemTemplate* tTemplate = mCatalog.find(pItem.templateId);

    pWidth = (tTemplate && tTemplate->width > 0) ? tTemplate->width : 1;
    pHeight = (tTemplate && tTemplate->height > 0) ? tTemplate->height : 1;
}

std::optional<size_t> Inventory::findIndexAt(ItemLocationType pLocation, uint32_t pSlot) const
{
    for (size_t tIndex = 0; tIndex < mItems.size(); ++tIndex)
    {
        if (mItems[tIndex].location == pLocation && mItems[tIndex].slot == pSlot)
        {
            return tIndex;
        }
    }

    return std::nullopt;
}

void Inventory::markOccupied(Occupancy& pOccupancy, uint32_t pSlot, uint32_t pWidth, uint32_t pHeight) const
{
    CellPosition tCell = toCell(pSlot);

    for (uint32_t tRow = 0; tRow < pHeight; ++tRow)
    {
        for (uint32_t tColumn = 0; tColumn < pWidth; ++tColumn)
        {
            uint32_t tX = tCell.x + tColumn;
            uint32_t tY = tCell.y + tRow;

            if (tX >= kGridWidth || tY >= kPageHeight)
            {
                continue; // clipped - a stored slot that no longer fits
            }

            pOccupancy[toSlot(tCell.page, tX, tY)] = true;
        }
    }
}

Inventory::Occupancy Inventory::buildOccupancy(size_t pIgnoredIndex) const
{
    Occupancy tOccupancy(kBagSlotCount, false);

    for (size_t tIndex = 0; tIndex < mItems.size(); ++tIndex)
    {
        if (tIndex == pIgnoredIndex || mItems[tIndex].location != ItemLocationType::eInventory)
        {
            continue;
        }

        if (mItems[tIndex].slot >= kBagSlotCount)
        {
            continue; // out of range (a shrunk grid) - nothing to mark
        }

        uint32_t tWidth = 0;
        uint32_t tHeight = 0;
        getFootprint(mItems[tIndex], tWidth, tHeight);

        markOccupied(tOccupancy, mItems[tIndex].slot, tWidth, tHeight);
    }

    return tOccupancy;
}

bool Inventory::canPlaceAt(uint32_t pSlot, uint32_t pWidth, uint32_t pHeight, const Occupancy& pOccupancy) const
{
    if (pSlot >= kBagSlotCount)
    {
        return false;
    }

    CellPosition tCell = toCell(pSlot);

    // Neither edge may be crossed. The page check is what keeps an item from
    // straddling two pages, which would make it unreachable from either.
    if (tCell.x + pWidth > kGridWidth || tCell.y + pHeight > kPageHeight)
    {
        return false;
    }

    for (uint32_t tRow = 0; tRow < pHeight; ++tRow)
    {
        for (uint32_t tColumn = 0; tColumn < pWidth; ++tColumn)
        {
            if (pOccupancy[toSlot(tCell.page, tCell.x + tColumn, tCell.y + tRow)])
            {
                return false;
            }
        }
    }

    return true;
}

std::optional<uint32_t> Inventory::findFreePlacement(uint32_t pWidth, uint32_t pHeight, const Occupancy& pOccupancy) const
{
    // First fit, scanning pages in order and each page left-to-right,
    // top-to-bottom - so items settle towards the start of the bag and the
    // player's first page stays the useful one.
    for (uint32_t tSlot = 0; tSlot < kBagSlotCount; ++tSlot)
    {
        if (canPlaceAt(tSlot, pWidth, pHeight, pOccupancy))
        {
            return tSlot;
        }
    }

    return std::nullopt;
}

uint32_t Inventory::addItem(uint32_t pTemplateId, uint32_t pCount)
{
    const ItemTemplate* tTemplate = mCatalog.find(pTemplateId);

    if (!tTemplate || pCount == 0)
    {
        return pCount; // unknown item - nothing placed
    }

    uint32_t tRemaining = pCount;

    // Existing stacks first, so ten potions stay one cell rather than ten.
    if (tTemplate->isStackable())
    {
        for (ItemInstance& tItem : mItems)
        {
            if (tRemaining == 0)
            {
                break;
            }

            if (tItem.location != ItemLocationType::eInventory || tItem.templateId != pTemplateId)
            {
                continue;
            }

            uint32_t tSpace = tTemplate->maxStack > tItem.count ? tTemplate->maxStack - tItem.count : 0;
            uint32_t tMoved = std::min(tSpace, tRemaining);

            tItem.count += tMoved;
            tRemaining -= tMoved;
        }
    }

    uint32_t tWidth = tTemplate->width > 0 ? tTemplate->width : 1;
    uint32_t tHeight = tTemplate->height > 0 ? tTemplate->height : 1;

    while (tRemaining > 0)
    {
        // Rebuilt each iteration because the previous one placed something.
        Occupancy tOccupancy = buildOccupancy();
        auto tPlacement = findFreePlacement(tWidth, tHeight, tOccupancy);

        if (!tPlacement)
        {
            break; // no rectangle fits - the caller is told via the return value
        }

        ItemInstance tNew;
        tNew.templateId = pTemplateId;
        tNew.count = std::min(tTemplate->maxStack, tRemaining);
        tNew.location = ItemLocationType::eInventory;
        tNew.slot = *tPlacement;

        tRemaining -= tNew.count;
        mItems.push_back(tNew);
    }

    return tRemaining;
}

uint32_t Inventory::removeItem(uint32_t pTemplateId, uint32_t pCount)
{
    uint32_t tRemaining = pCount;

    for (auto tIterator = mItems.begin(); tIterator != mItems.end() && tRemaining > 0; )
    {
        // Equipped items are deliberately not consumed - "you have 1 sword"
        // must not be satisfied by the one you are holding.
        if (tIterator->location != ItemLocationType::eInventory || tIterator->templateId != pTemplateId)
        {
            ++tIterator;
            continue;
        }

        uint32_t tTaken = std::min(tIterator->count, tRemaining);
        tIterator->count -= tTaken;
        tRemaining -= tTaken;

        if (tIterator->count == 0)
        {
            tIterator = mItems.erase(tIterator);
        }
        else
        {
            ++tIterator;
        }
    }

    return pCount - tRemaining;
}

uint32_t Inventory::countItem(uint32_t pTemplateId) const
{
    uint32_t tTotal = 0;

    for (const ItemInstance& tItem : mItems)
    {
        if (tItem.location == ItemLocationType::eInventory && tItem.templateId == pTemplateId)
        {
            tTotal += tItem.count;
        }
    }

    return tTotal;
}

bool Inventory::equip(uint32_t pBagSlot, uint32_t pCharacterLevel)
{
    auto tIndex = findIndexAt(ItemLocationType::eInventory, pBagSlot);

    if (!tIndex)
    {
        return false;
    }

    const ItemTemplate* tTemplate = mCatalog.find(mItems[*tIndex].templateId);

    if (!tTemplate || !tTemplate->isEquippable() || pCharacterLevel < tTemplate->requiredLevel)
    {
        return false;
    }

    uint32_t tTargetSlot = static_cast<uint32_t>(tTemplate->equipSlot);
    auto tWornIndex = findIndexAt(ItemLocationType::eEquipped, tTargetSlot);

    // The item coming off has to find a rectangle among the cells the item
    // going on is vacating, plus whatever was already free. It is NOT enough
    // to reuse the vacated slot: a 1x3 sword coming off cannot go where a 2x2
    // helmet just left. If it does not fit anywhere, the whole swap is
    // refused rather than half-applied.
    std::optional<uint32_t> tWornPlacement;

    if (tWornIndex)
    {
        Occupancy tOccupancy = buildOccupancy(*tIndex);

        uint32_t tWornWidth = 0;
        uint32_t tWornHeight = 0;
        getFootprint(mItems[*tWornIndex], tWornWidth, tWornHeight);

        tWornPlacement = findFreePlacement(tWornWidth, tWornHeight, tOccupancy);

        if (!tWornPlacement)
        {
            return false;
        }
    }

    if (tWornIndex)
    {
        mItems[*tWornIndex].location = ItemLocationType::eInventory;
        mItems[*tWornIndex].slot = *tWornPlacement;
    }

    mItems[*tIndex].location = ItemLocationType::eEquipped;
    mItems[*tIndex].slot = tTargetSlot;

    return true;
}

bool Inventory::unequip(EquipSlotType pSlot, std::optional<uint32_t> pBagSlot)
{
    auto tIndex = findIndexAt(ItemLocationType::eEquipped, static_cast<uint32_t>(pSlot));

    if (!tIndex)
    {
        return false;
    }

    Occupancy tOccupancy = buildOccupancy();

    uint32_t tWidth = 0;
    uint32_t tHeight = 0;
    getFootprint(mItems[*tIndex], tWidth, tHeight);

    std::optional<uint32_t> tPlacement;

    if (pBagSlot)
    {
        if (canPlaceAt(*pBagSlot, tWidth, tHeight, tOccupancy))
        {
            tPlacement = pBagSlot;
        }
    }
    else
    {
        tPlacement = findFreePlacement(tWidth, tHeight, tOccupancy);
    }

    if (!tPlacement)
    {
        return false; // no room shaped like this item
    }

    mItems[*tIndex].location = ItemLocationType::eInventory;
    mItems[*tIndex].slot = *tPlacement;

    return true;
}

bool Inventory::moveItem(uint32_t pFromSlot, uint32_t pToSlot)
{
    auto tFromIndex = findIndexAt(ItemLocationType::eInventory, pFromSlot);

    if (!tFromIndex)
    {
        return false;
    }

    if (pFromSlot == pToSlot)
    {
        return true;
    }

    uint32_t tWidth = 0;
    uint32_t tHeight = 0;
    getFootprint(mItems[*tFromIndex], tWidth, tHeight);

    Occupancy tOccupancyWithoutSelf = buildOccupancy(*tFromIndex);

    if (!canPlaceAt(pToSlot, tWidth, tHeight, tOccupancyWithoutSelf))
    {
        return false;
    }

    mItems[*tFromIndex].slot = pToSlot;
    return true;
}

const ItemInstance* Inventory::getEquipped(EquipSlotType pSlot) const
{
    auto tIndex = findIndexAt(ItemLocationType::eEquipped, static_cast<uint32_t>(pSlot));
    return tIndex ? &mItems[*tIndex] : nullptr;
}

Inventory::EquipmentBonuses Inventory::getEquipmentBonuses() const
{
    EquipmentBonuses tBonuses;

    for (const ItemInstance& tItem : mItems)
    {
        if (tItem.location != ItemLocationType::eEquipped)
        {
            continue;
        }

        const ItemTemplate* tTemplate = mCatalog.find(tItem.templateId);

        if (!tTemplate)
        {
            continue;
        }

        tBonuses.attack += tTemplate->bonusAttack;
        tBonuses.defense += tTemplate->bonusDefense;
        tBonuses.maxHealth += tTemplate->bonusMaxHealth;
        tBonuses.maxMana += tTemplate->bonusMaxMana;
    }

    return tBonuses;
}
