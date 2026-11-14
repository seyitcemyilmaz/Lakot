#ifndef LAKOT_ENTITY_RENDERER_H
#define LAKOT_ENTITY_RENDERER_H

#include <optional>

#include <glm/glm.hpp>

namespace lakot
{

struct WorldEntity;

class EquipmentAttachments;
class ItemAppearance;
class ShaderProgram;
class WorldEntities;

class EntityRenderer
{
public:
    EntityRenderer(ItemAppearance& pAppearance, EquipmentAttachments& pAttachments);

    struct WeaponEdge
    {
        glm::vec3 tip;
        glm::vec3 base;
    };

    std::optional<WeaponEdge> findWeaponEdge(const WorldEntity& pEntity, double pExtraSeconds) const;

    void draw(ShaderProgram& pShader, const WorldEntities& pEntities, double pExtraSeconds) const;

private:
    static glm::mat4 getModelTransform(const WorldEntity& pEntity, double pExtraSeconds);
    void drawGroundItem(ShaderProgram& pShader, const WorldEntity& pEntity) const;

    ItemAppearance& mAppearance;
    EquipmentAttachments& mAttachments;
};

}

#endif
