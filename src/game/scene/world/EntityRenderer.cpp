#include "EntityRenderer.h"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

#include "EquipmentAttachments.h"
#include "WorldEntities.h"
#include "WorldViewConstants.h"

#include "../../graphics/render/ShaderProgram.h"
#include "../../item/ItemAppearance.h"

using namespace lakot;

namespace
{
    constexpr float kGroundItemSize = 0.8f;
    constexpr float kGroundItemLift = 0.45f;

    void drawModel(ShaderProgram& pShader, const SkinnedModel& pModel, const glm::mat4& pTransform, const std::vector<glm::mat4>& pPalette)
    {
        pShader.setMat4("uModel", pTransform);
        pShader.setMat4Array("uBones", pPalette.data(), static_cast<unsigned int>(pPalette.size()));
        pModel.draw(0);
    }
}

EntityRenderer::EntityRenderer(ItemAppearance& pAppearance, EquipmentAttachments& pAttachments)
    : mAppearance(pAppearance)
    , mAttachments(pAttachments)
{

}

void EntityRenderer::drawGroundItem(ShaderProgram& pShader, const WorldEntity& pEntity) const
{
    const ItemLook* tLook = pEntity.gold > 0 ? nullptr : mAppearance.find({ pEntity.itemTemplateId, 0 });

    if (!tLook)
    {
        return;
    }

    glm::vec3 tSize = tLook->model->getBounds().getSize();
    float tLongest = std::max({ tSize.x, tSize.y, tSize.z, 0.001f });
    float tScale = kGroundItemSize / tLongest;

    glm::mat4 tTransform = glm::translate(glm::mat4(1.0f), pEntity.targetPosition + glm::vec3(0.0f, kGroundItemLift, 0.0f));
    tTransform = glm::rotate(tTransform, static_cast<float>(pEntity.id % 628) * 0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
    tTransform = glm::scale(tTransform, glm::vec3(tScale));
    tTransform = glm::translate(tTransform, -(tLook->model->getBounds().min + tSize * 0.5f));

    ItemAppearance::apply(pShader, *tLook);
    drawModel(pShader, *tLook->model, tTransform, tLook->model->getBindPalette());
}

void EntityRenderer::draw(ShaderProgram& pShader, const WorldEntities& pEntities, double pExtraSeconds) const
{
    const float tHeight = WorldView::kPlayerBoxHalfExtents.y * 2.0f;
    const glm::vec3 tToFeet(0.0f, WorldView::kPlayerBoxHalfExtents.y, 0.0f);

    for (const auto& [tId, tEntity] : pEntities.getAll())
    {
        if (tEntity.type == EntityType::eGroundItem)
        {
            drawGroundItem(pShader, tEntity);
            continue;
        }

        if (!tEntity.model || !tEntity.animator)
        {
            continue;
        }

        glm::vec3 tFeet = WorldEntities::getPosition(tEntity, pExtraSeconds) - tToFeet;
        float tScale = tHeight / tEntity.model->getHeight() * tEntity.scale;

        glm::mat4 tTransform = glm::translate(glm::mat4(1.0f), tFeet);
        tTransform = glm::rotate(tTransform, tEntity.facing, glm::vec3(0.0f, 1.0f, 0.0f));
        tTransform = glm::scale(tTransform, glm::vec3(tScale));
        tTransform = glm::translate(tTransform, glm::vec3(0.0f, -tEntity.model->getBounds().min.y, 0.0f));

        const std::vector<glm::mat4>& tPalette = tEntity.animator->getPalette();

        ItemAppearance::apply(pShader, ItemLook{ nullptr, tEntity.tint, 0.0f });
        drawModel(pShader, *tEntity.model, tTransform, tPalette);

        for (const EquippedVisual& tEquipped : tEntity.equipment)
        {
            if (tEquipped.slot == EquipSlotType::eArmor)
            {
                continue;
            }

            const ItemLook* tLook = mAppearance.find(tEquipped.item);

            if (!tLook)
            {
                continue;
            }

            const EquipmentAttachments::Placement* tPlacement = mAttachments.find(tEquipped.slot, *tEntity.model, *tLook->model);

            if (!tPlacement)
            {
                continue;
            }

            ItemAppearance::apply(pShader, *tLook);
            drawModel(pShader, *tLook->model, tTransform * tPalette[static_cast<size_t>(tPlacement->joint)] * tPlacement->bindTransform,
                      tLook->model->getBindPalette());
        }
    }
}
