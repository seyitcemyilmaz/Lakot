#ifndef LAKOT_ITEM_ICON_PATH_H
#define LAKOT_ITEM_ICON_PATH_H

#include <string>

#include "../network/GameTypes.h"

namespace lakot
{

class ItemIconRenderer;

class ItemIconPath
{
public:
    static std::string resolve(ItemIconRenderer& pItemIcons, const ItemTemplate* pTemplate);

private:
    ItemIconPath() = delete;
    ~ItemIconPath() = delete;
};

}

#endif
