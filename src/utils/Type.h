#ifndef LAKOT_TYPE_H
#define LAKOT_TYPE_H

namespace lakot
{

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
    eTerrain,
    eModel,
    eSkinnedModel
};

enum class CameraType
{
    eUndefined,
    eFPS,
    eThirdPerson
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
