#ifndef LAKOT_RENDERER_H
#define LAKOT_RENDERER_H

#include <lakot/abstract/render/ARenderer.h>

namespace lakot {

class Renderer : public ARenderer
{
public:
    virtual ~Renderer() override;
    Renderer();
};

}

#endif
