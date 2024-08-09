#ifndef LAKOT_IRENDERER_H
#define LAKOT_IRENDERER_H

#include "../../engine/IObject.h"

#include "IRenderable.h"

namespace lakot {

class IRenderer : public IObject
{
public:
    virtual ~IRenderer() = default;
    IRenderer() = default;

    virtual void render(IRenderable* pRenderable) = 0;
};

}

#endif
