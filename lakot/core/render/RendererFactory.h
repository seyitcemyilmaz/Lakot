#ifndef LAKOT_RENDERERFACTORY_H
#define LAKOT_RENDERERFACTORY_H

#include <lakot/abstract/render/ARenderer.h>

namespace lakot {

class RendererFactory
{
public:
    virtual ~RendererFactory() = default;
    RendererFactory() = default;

    static ARenderer* createRenderer();
};

}

#endif
