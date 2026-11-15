#ifndef LAKOT_INVENTORY_UNITS_H
#define LAKOT_INVENTORY_UNITS_H

#include <algorithm>
#include <cstdint>
#include <string>

#include <RmlUi/Core/Types.h>

#include "../../network/GameTypes.h"

namespace lakot::InventoryUnits
{

inline Rml::String toPixels(int pValue)
{
    return Rml::String(std::to_string(pValue) + "px");
}

inline Rml::String toDp(uint32_t pValue)
{
    return Rml::String(std::to_string(pValue) + "dp");
}

inline uint32_t getFootprintWidth(const ItemTemplate* pTemplate)
{
    return pTemplate && pTemplate->width > 0 ? pTemplate->width : 1;
}

inline uint32_t getFootprintHeight(const ItemTemplate* pTemplate)
{
    return pTemplate && pTemplate->height > 0 ? pTemplate->height : 1;
}

struct IconFit
{
    uint32_t left{0};
    uint32_t top{0};
    uint32_t width{0};
    uint32_t height{0};
};

inline IconFit fitContain(const ItemTemplate* pTemplate, uint32_t pBoxWidth, uint32_t pBoxHeight)
{
    const float tAspect = static_cast<float>(getFootprintWidth(pTemplate)) / static_cast<float>(getFootprintHeight(pTemplate));
    float tWidth = static_cast<float>(pBoxWidth);
    float tHeight = tWidth / tAspect;

    if (tHeight > static_cast<float>(pBoxHeight))
    {
        tHeight = static_cast<float>(pBoxHeight);
        tWidth = tHeight * tAspect;
    }

    IconFit tFit;
    tFit.width = std::max<uint32_t>(1, static_cast<uint32_t>(tWidth + 0.5f));
    tFit.height = std::max<uint32_t>(1, static_cast<uint32_t>(tHeight + 0.5f));
    tFit.left = (pBoxWidth - std::min(pBoxWidth, tFit.width)) / 2;
    tFit.top = (pBoxHeight - std::min(pBoxHeight, tFit.height)) / 2;
    return tFit;
}

inline Rml::String toFitStyle(const IconFit& pFit)
{
    return "position: absolute; left: " + std::to_string(pFit.left) + "dp; top: " + std::to_string(pFit.top)
        + "dp; width: " + std::to_string(pFit.width) + "dp; height: " + std::to_string(pFit.height) + "dp;";
}

}

#endif
