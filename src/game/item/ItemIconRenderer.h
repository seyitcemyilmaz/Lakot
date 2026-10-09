#ifndef LAKOT_ITEM_ICON_RENDERER_H
#define LAKOT_ITEM_ICON_RENDERER_H

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

#include "../network/GameTypes.h"

namespace lakot
{

class InventoryController;
class ItemAppearance;
class RmlUiLayer;
class ShaderProgram;
struct ItemLook;

// Draws an item's inventory icon from the same model, shader and light it is worn with.
class ItemIconRenderer
{
public:
    ItemIconRenderer(ItemAppearance& pAppearance, const InventoryController& pInventoryController,
                     ShaderProgram& pShader, RmlUiLayer& pRmlUiLayer);

    // An RML image source, rendered on first use; empty when the item has no model.
    std::string getIconSource(const ItemVisual& pVisual);

private:
    static constexpr int kPixelsPerCell = 64;
    static constexpr int kSupersampling = 2;

    ItemAppearance& mAppearance;
    const InventoryController& mInventoryController;
    ShaderProgram& mShader;
    RmlUiLayer& mRmlUiLayer;

    std::unordered_set<std::string> mRendered;

    std::vector<uint8_t> render(const ItemLook& pLook, int pWidth, int pHeight) const;
};

}

#endif
