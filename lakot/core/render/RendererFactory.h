#ifndef LAKOT_RENDERERFACTORY_H
#define LAKOT_RENDERERFACTORY_H

#include <lakot/interface/graphics/api/IGraphicsAPI.h>

#include "Renderer.h"

namespace lakot {

class RendererFactory
{
public:
    virtual ~RendererFactory() = default;
    RendererFactory() = default;

    static Renderer* createRenderer(GraphicsAPIType pGraphicsAPIType);
};

}

#endif
