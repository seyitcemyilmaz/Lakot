#include "Character.h"

#include <algorithm>
#include <utility>

#include "ItemCatalog.h"

using namespace lakot;

Character::Character(uint64_t pCharacterId, std::string pName, const ItemCatalog& pCatalog)
    : mCharacterId(pCharacterId)
    , mName(std::move(pName))
    , mInventory(pCatalog)
{

}

uint64_t Character::getId() const
{
    return mCharacterId;
}

const std::string& Character::getName() const
{
    return mName;
}

const CharacterStats& Character::getStats() const
{
    return mStats;
}

const Inventory& Character::getInventory() const
{
    return mInventory;
}

int32_t Character::getMaxHealth() const
{
    return StatFormula::getMaxHealth(mStats, mInventory.getEquipmentBonuses().maxHealth);
}

int32_t Character::getMaxMana() const
{
    return StatFormula::getMaxMana(mStats, mInventory.getEquipmentBonuses().maxMana);
}

int32_t Character::getAttack() const
{
    return StatFormula::getAttack(mStats, mInventory.getEquipmentBonuses().attack);
}

int32_t Character::getDefense() const
{
    return StatFormula::getDefense(mStats, mInventory.getEquipmentBonuses().defense);
}

uint32_t Character::getKingdom() const
{
    return mKingdom;
}

void Character::setKingdom(uint32_t pKingdom)
{
    mKingdom = pKingdom;
}

bool Character::isDead() const
{
    return mStats.health <= 0;
}

bool Character::takeDamage(int32_t pAmount)
{
    if (isDead() || pAmount <= 0)
    {
        return false;
    }

    mStats.health = std::max(0, mStats.health - pAmount);
    markDirty();
    return isDead();
}

void Character::revive()
{
    mStats.health = getMaxHealth();
    markDirty();
}

void Character::setStats(const CharacterStats& pStats)
{
    mStats = pStats;
    refreshVitals();
}

void Character::setItems(std::vector<ItemInstance> pItems)
{
    mInventory.setItems(std::move(pItems));

    // Equipment feeds the maximums, so loading it can move them.
    refreshVitals();
}

void Character::setHealth(int32_t pHealth)
{
    mStats.health = std::clamp(pHealth, 0, getMaxHealth());
    markDirty();
}

void Character::setMana(int32_t pMana)
{
    mStats.mana = std::clamp(pMana, 0, getMaxMana());
    markDirty();
}

void Character::setLevel(uint32_t pLevel)
{
    mStats.level = std::max(1u, pLevel);
    mStats.experience = 0;
    refreshVitals();
    markDirty();
}

uint32_t Character::giveItem(uint32_t pTemplateId, uint32_t pCount)
{
    uint32_t tLeftOver = mInventory.addItem(pTemplateId, pCount);

    if (tLeftOver != pCount)
    {
        markDirty();
    }

    return tLeftOver;
}

uint32_t Character::takeItem(uint32_t pTemplateId, uint32_t pCount)
{
    uint32_t tRemoved = mInventory.removeItem(pTemplateId, pCount);

    if (tRemoved > 0)
    {
        markDirty();
    }

    return tRemoved;
}

uint32_t Character::countItem(uint32_t pTemplateId) const
{
    return mInventory.countItem(pTemplateId);
}

ConsumeResultType Character::useConsumable(const ItemTemplate& pTemplate)
{
    if (!pTemplate.isUsable())
    {
        return ConsumeResultType::eNotUsable;
    }

    if (mInventory.countItem(pTemplate.templateId) == 0)
    {
        return ConsumeResultType::eNotOwned;
    }

    bool tHealsHealth = pTemplate.restoreHealth > 0 && mStats.health < getMaxHealth();
    bool tHealsMana = pTemplate.restoreMana > 0 && mStats.mana < getMaxMana();

    if (!tHealsHealth && !tHealsMana)
    {
        return ConsumeResultType::eFull;
    }

    if (takeItem(pTemplate.templateId, 1) != 1)
    {
        return ConsumeResultType::eNotOwned;
    }

    mStats.health = static_cast<int32_t>(std::min<int64_t>(int64_t{mStats.health} + pTemplate.restoreHealth, getMaxHealth()));
    mStats.mana = static_cast<int32_t>(std::min<int64_t>(int64_t{mStats.mana} + pTemplate.restoreMana, getMaxMana()));
    refreshVitals();
    markDirty();
    return ConsumeResultType::eConsumed;
}

bool Character::equipItem(uint32_t pBagSlot)
{
    if (!mInventory.equip(pBagSlot, mStats.level))
    {
        return false;
    }

    refreshVitals();
    markDirty();
    return true;
}

bool Character::unequipItem(EquipSlotType pSlot, std::optional<uint32_t> pBagSlot)
{
    if (!mInventory.unequip(pSlot, pBagSlot))
    {
        return false;
    }

    refreshVitals();
    markDirty();
    return true;
}

bool Character::moveItem(uint32_t pFromSlot, uint32_t pToSlot)
{
    if (!mInventory.moveItem(pFromSlot, pToSlot))
    {
        return false;
    }

    markDirty();
    return true;
}

uint32_t Character::addExperience(uint64_t pAmount)
{
    if (pAmount == 0)
    {
        return 0;
    }

    mStats.experience += pAmount;

    uint32_t tLevelsGained = 0;

    // A loop, not a single check: a large reward (a quest turn-in, a boss)
    // can legitimately cover several levels at once, and swallowing the
    // remainder would quietly lose progress.
    while (true)
    {
        uint64_t tNeeded = StatFormula::getExperienceForNextLevel(mStats.level);

        if (mStats.experience < tNeeded)
        {
            break;
        }

        mStats.experience -= tNeeded;
        mStats.level++;
        tLevelsGained++;
    }

    if (tLevelsGained > 0)
    {
        // Levelling raises the maximums; a level-up heals to full, which is
        // the near-universal convention and avoids the awkward state where
        // gaining a level leaves you proportionally more wounded.
        StatFormula::initializeVitals(mStats);
        refreshVitals();
    }

    markDirty();

    return tLevelsGained;
}

void Character::addGold(int64_t pAmount)
{
    if (pAmount < 0)
    {
        uint64_t tCost = static_cast<uint64_t>(-pAmount);
        mStats.gold = tCost > mStats.gold ? 0 : mStats.gold - tCost;
    }
    else
    {
        mStats.gold += static_cast<uint64_t>(pAmount);
    }

    markDirty();
}

void Character::refreshVitals()
{
    StatFormula::clampVitals(mStats, getMaxHealth(), getMaxMana());
}

bool Character::isDirty() const
{
    return mIsDirty;
}

void Character::clearDirty()
{
    mIsDirty = false;
}

void Character::markDirty()
{
    mIsDirty = true;
}
