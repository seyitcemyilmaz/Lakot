#ifndef LAKOT_IRENDERABLE_H
#define LAKOT_IRENDERABLE_H

#include "../../engine/IObject.h"

namespace lakot {

enum class RenderableType
{
    eUnknown,
    eMesh
};

class IRenderable : public IObject
{
public:
    virtual ~IRenderable() = default;
    IRenderable() = default;

    virtual RenderableType getType() const = 0;
};

}

#endif
