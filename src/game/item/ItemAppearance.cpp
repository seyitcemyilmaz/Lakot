#include "ItemAppearance.h"

#include "../asset/AssetManager.h"
#include "../graphics/render/ShaderProgram.h"
#include "../network/controller/InventoryController.h"

using namespace lakot;

ItemAppearance::ItemAppearance(AssetManager& pAssets, const InventoryController& pInventoryController)
    : mAssets(pAssets)
    , mInventoryController(pInventoryController)
{

}

const ItemLook* ItemAppearance::find(const ItemVisual& pVisual)
{
    uint64_t tKey = (static_cast<uint64_t>(pVisual.templateId) << 32) | pVisual.upgradeLevel;
    auto tIterator = mLooks.find(tKey);

    if (tIterator == mLooks.end())
    {
        const ItemTemplate* tTemplate = mInventoryController.findTemplate(pVisual.templateId);

        if (!tTemplate)
        {
            return nullptr;
        }

        ItemLook tLook;

        if (!tTemplate->model.empty())
        {
            tLook.model = mAssets.getSkinnedModel(tTemplate->model);
        }

        tLook.tint = glm::vec3((tTemplate->tint >> 16) & 0xFF, (tTemplate->tint >> 8) & 0xFF, tTemplate->tint & 0xFF) / 255.0f;

        tIterator = mLooks.emplace(tKey, std::move(tLook)).first;
    }

    return tIterator->second.model ? &tIterator->second : nullptr;
}

void ItemAppearance::apply(ShaderProgram& pShader, const ItemLook& pLook)
{
    pShader.setVec3("uTint", pLook.tint);
    pShader.setFloat("uGlow", pLook.glow);
}
