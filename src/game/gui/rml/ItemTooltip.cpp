#include "ItemTooltip.h"

#include <algorithm>
#include <string>

#include <SDL3/SDL.h>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/DataStructHandle.h>
#include <RmlUi/Core/ElementDocument.h>

#include "ClassCatalog.h"
#include "InventoryUnits.h"
#include "RmlUiLayer.h"
#include "../../localization/Localization.h"

using namespace lakot;

namespace
{
    constexpr float kScreenMargin = 4.0f;
}

ItemTooltip::ItemTooltip(RmlUiLayer& pRmlUiLayer, const Localization& pLocalization, const ClassCatalog& pClassCatalog, const RmlDocument& pDocument)
    : mRmlUiLayer(pRmlUiLayer)
    , mLocalization(pLocalization)
    , mClassCatalog(pClassCatalog)
    , mDocument(pDocument)
{
}

void ItemTooltip::bind(Rml::DataModelConstructor& pConstructor)
{
    static bool sTypesRegistered = false;

    if (!sTypesRegistered)
    {
        auto tLine = pConstructor.RegisterStruct<TooltipLine>();
        tLine.RegisterMember("text", &TooltipLine::text);
        pConstructor.RegisterArray<std::vector<TooltipLine>>();

        sTypesRegistered = true;
    }

    pConstructor.Bind("tooltip_lines", &mLines);
    pConstructor.Bind("tooltip_name", &mName);
    pConstructor.Bind("tooltip_hint", &mHint);
    pConstructor.Bind("tooltip_left", &mLeft);
    pConstructor.Bind("tooltip_top", &mTop);
    pConstructor.Bind("has_tooltip", &mIsVisible);

    mModelHandle = pConstructor.GetModelHandle();
}

bool ItemTooltip::isEquippable(const ItemTemplate* pTemplate)
{
    return pTemplate && pTemplate->equipSlot != EquipSlotType::eNone;
}

std::string ItemTooltip::getSlotRoleName(EquipSlotType pSlot) const
{
    switch (pSlot)
    {
        case EquipSlotType::eWeapon: return mLocalization.text("inventory.slot.weapon");
        case EquipSlotType::eArmor:  return mLocalization.text("inventory.slot.armor");
        case EquipSlotType::eHelmet: return mLocalization.text("inventory.slot.helmet");
        case EquipSlotType::eBoots:  return mLocalization.text("inventory.slot.boots");
        default:                     return std::string();
    }
}

void ItemTooltip::showItem(const ItemTemplate* pTemplate, uint32_t pCount, bool pIsEquipped)
{
    mLines.clear();

    if (!pTemplate)
    {
        mName = mLocalization.text("inventory.unknown_item");
        mHint = "";
    }
    else
    {
        mName = mLocalization.content("item." + std::to_string(pTemplate->templateId) + ".name", pTemplate->name);

        if (pCount > 1)
        {
            mLines.push_back({ mLocalization.format("inventory.tooltip.quantity", { { "count", std::to_string(pCount) } }) });
        }

        if (pTemplate->bonusAttack != 0)
            mLines.push_back({ mLocalization.format("inventory.tooltip.bonus_attack", { { "value", std::to_string(pTemplate->bonusAttack) } }) });
        if (pTemplate->bonusDefense != 0)
            mLines.push_back({ mLocalization.format("inventory.tooltip.bonus_defense", { { "value", std::to_string(pTemplate->bonusDefense) } }) });
        if (pTemplate->bonusMaxHealth != 0)
            mLines.push_back({ mLocalization.format("inventory.tooltip.bonus_max_health", { { "value", std::to_string(pTemplate->bonusMaxHealth) } }) });
        if (pTemplate->bonusMaxMana != 0)
            mLines.push_back({ mLocalization.format("inventory.tooltip.bonus_max_mana", { { "value", std::to_string(pTemplate->bonusMaxMana) } }) });

        if (pTemplate->requiredLevel > 1)
            mLines.push_back({ mLocalization.format("inventory.tooltip.requires_level", { { "level", std::to_string(pTemplate->requiredLevel) } }) });

        if (!pTemplate->allowedClasses.empty())
        {
            std::string tClasses;

            for (uint32_t tClassId : pTemplate->allowedClasses)
            {
                const ClassDefinition* tClass = mClassCatalog.find(tClassId);
                tClasses += (tClasses.empty() ? "" : ", ") + (tClass ? mLocalization.content("class." + tClass->key + ".name", tClass->name) : std::string("?"));
            }

            mLines.push_back({ mLocalization.format("inventory.tooltip.classes", { { "classes", tClasses } }) });
        }

        if (pIsEquipped)
        {
            mHint = mLocalization.text("inventory.hint.unequip");
        }
        else if (isEquippable(pTemplate))
        {
            mHint = mLocalization.text("inventory.hint.equip");
        }
        else
        {
            mHint = "";
        }
    }

    reveal();
}

void ItemTooltip::showEmptySlot(EquipSlotType pSlot)
{
    mLines.clear();
    mName = getSlotRoleName(pSlot);
    mHint = mLocalization.text("inventory.hint.empty");

    reveal();
}

void ItemTooltip::hide()
{
    if (!mIsVisible)
    {
        return;
    }

    mIsVisible = false;
    mModelHandle.DirtyAllVariables();
}

void ItemTooltip::update(float pMouseX, float pMouseY)
{
    if (mIsVisible)
    {
        position(pMouseX, pMouseY);
    }
}

void ItemTooltip::forget()
{
    mIsVisible = false;
}

void ItemTooltip::reveal()
{
    mIsVisible = true;
    mModelHandle.DirtyAllVariables();

    float tMouseX = 0.0f;
    float tMouseY = 0.0f;
    SDL_GetMouseState(&tMouseX, &tMouseY);
    position(tMouseX, tMouseY);
}

void ItemTooltip::position(float pMouseX, float pMouseY)
{
    if (!mDocument)
    {
        return;
    }

    Rml::Vector2i tScreen = mRmlUiLayer.getContext()->GetDimensions();

    float tWidth = 200.0f;
    float tHeight = 0.0f;

    if (Rml::Element* tTooltip = mDocument->QuerySelector(".inv-tooltip"))
    {
        tWidth = std::max(tWidth, tTooltip->GetOffsetWidth());
        tHeight = tTooltip->GetOffsetHeight();
    }

    float tLeft = pMouseX + 18.0f;
    float tTop = pMouseY + 8.0f;

    if (tLeft + tWidth > tScreen.x - kScreenMargin)
    {
        tLeft = pMouseX - 12.0f - tWidth;
    }

    tTop = std::min(tTop, tScreen.y - kScreenMargin - tHeight);
    tTop = std::max(tTop, kScreenMargin);
    tLeft = std::max(tLeft, kScreenMargin);

    Rml::String tNewLeft = InventoryUnits::toPixels(static_cast<int>(tLeft - mDocument->GetAbsoluteLeft()));
    Rml::String tNewTop = InventoryUnits::toPixels(static_cast<int>(tTop - mDocument->GetAbsoluteTop()));

    if (tNewLeft != mLeft || tNewTop != mTop)
    {
        mLeft = tNewLeft;
        mTop = tNewTop;
        mModelHandle.DirtyVariable("tooltip_left");
        mModelHandle.DirtyVariable("tooltip_top");
    }
}
