#ifndef LAKOT_INVENTORY_UNITS_H
#define LAKOT_INVENTORY_UNITS_H

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

}

#endif
