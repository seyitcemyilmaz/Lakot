#ifndef LAKOT_TYPE_H
#define LAKOT_TYPE_H

namespace lakot
{

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

enum class ShaderType
{
    eUndefined,
    eVertex,
    eFragment
};

}

#endif
