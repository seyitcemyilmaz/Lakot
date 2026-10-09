#ifndef LAKOT_ITEM_APPEARANCE_H
#define LAKOT_ITEM_APPEARANCE_H

#include <cstdint>
#include <memory>
#include <unordered_map>

#include <glm/glm.hpp>

#include "../graphics/model/SkinnedModel.h"
#include "../network/GameTypes.h"

namespace lakot
{

class AssetManager;
class InventoryController;
class ShaderProgram;

struct ItemLook
{
    std::shared_ptr<SkinnedModel> model;
    glm::vec3 tint{1.0f};
    float glow{0.0f};
};

// The one place that decides how an item looks, shared by the world and the inventory icons.
class ItemAppearance
{
public:
    ItemAppearance(AssetManager& pAssets, const InventoryController& pInventoryController);

    // nullptr when the item has no model, or its template has not arrived yet.
    const ItemLook* find(const ItemVisual& pVisual);

    static void apply(ShaderProgram& pShader, const ItemLook& pLook);

private:
    AssetManager& mAssets;
    const InventoryController& mInventoryController;

    std::unordered_map<uint64_t, ItemLook> mLooks;
};

}

#endif
