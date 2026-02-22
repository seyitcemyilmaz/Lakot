#ifndef LAKOT_TYPE_H
#define LAKOT_TYPE_H

namespace lakot
{

enum class AssetType
{
    eUndefined,
    eModel,
    eNode,
    eMesh,
    eBone,
    eMaterial,
    eTexture
};

enum class EntityType
{
    eUndefined,
    eModel,
    eNode,
    eMesh,
    eBone,
    eMaterial,
    eTexture
};

enum class ButtonType
{
    eUndefined,
    eLeft,
    eRight,
    eMiddle
};

enum class RenderableType
{
    eUndefined,
    eBoxContainer,
    eTerrain
};

enum class CameraType
{
    eUndefined,
    eFPS
};

enum class GraphicsAPIType
{
    eUndefined,
    eOpenGL
};

enum class OpenGLType
{
    eUndefined,
    eCore,
    eES
};

enum class ShaderType
{
    eUndefined,
    eVertex,
    eFragment
};

enum class CharacterType
{
    eWarrior
};

enum class ItemType
{
    eArmor,
    eShield,
    eHelmet,
    eEarring,
    eNecklace,
    eBracelet,
    eShoe
};

}

#endif
