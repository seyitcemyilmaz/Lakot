#ifndef LAKOT_ENTITY_RENDERER_H
#define LAKOT_ENTITY_RENDERER_H

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

    void draw(ShaderProgram& pShader, const WorldEntities& pEntities, double pExtraSeconds) const;

private:
    void drawGroundItem(ShaderProgram& pShader, const WorldEntity& pEntity) const;

    ItemAppearance& mAppearance;
    EquipmentAttachments& mAttachments;
};

}

#endif
