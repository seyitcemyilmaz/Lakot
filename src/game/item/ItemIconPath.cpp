#include "ItemIconPath.h"

#include "ItemIconRenderer.h"

using namespace lakot;

std::string ItemIconPath::resolve(ItemIconRenderer& pItemIcons, const ItemTemplate* pTemplate)
{
    if (!pTemplate)
    {
        return std::string();
    }

    if (!pTemplate->model.empty())
    {
        return pItemIcons.getIconSource({ pTemplate->templateId, 0 });
    }

    return pTemplate->icon.empty() ? std::string() : "textures/items/" + pTemplate->icon;
}
