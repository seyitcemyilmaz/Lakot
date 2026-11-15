#ifndef LAKOT_ITEM_TOOLTIP_H
#define LAKOT_ITEM_TOOLTIP_H

#include <cstdint>
#include <vector>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

#include "RmlDocument.h"

#include "../../network/GameTypes.h"

namespace Rml
{
class DataModelConstructor;
}

namespace lakot
{

class ClassCatalog;
class Localization;
class RmlUiLayer;

class ItemTooltip
{
public:
    ItemTooltip(RmlUiLayer& pRmlUiLayer, const Localization& pLocalization, const ClassCatalog& pClassCatalog, const RmlDocument& pDocument);

    ItemTooltip(const ItemTooltip&) = delete;
    ItemTooltip& operator=(const ItemTooltip&) = delete;

    void bind(Rml::DataModelConstructor& pConstructor);

    void showItem(const ItemTemplate* pTemplate, uint32_t pCount, bool pIsEquipped);
    void showEmptySlot(EquipSlotType pSlot);
    void hide();
    void forget();
    void update(float pMouseX, float pMouseY);

private:
    struct TooltipLine
    {
        Rml::String text;
    };

    RmlUiLayer& mRmlUiLayer;
    const Localization& mLocalization;
    const ClassCatalog& mClassCatalog;
    const RmlDocument& mDocument;

    Rml::DataModelHandle mModelHandle;

    std::vector<TooltipLine> mLines;
    Rml::String mName;
    Rml::String mHint;
    Rml::String mLeft{"0px"};
    Rml::String mTop{"0px"};
    bool mIsVisible{false};

    void reveal();
    void position(float pMouseX, float pMouseY);
    std::string getSlotRoleName(EquipSlotType pSlot) const;
    static bool isEquippable(const ItemTemplate* pTemplate);
};

}

#endif
