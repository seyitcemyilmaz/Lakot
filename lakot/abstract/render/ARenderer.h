#ifndef LAKOT_ARENDERER_H
#define LAKOT_ARENDERER_H

#include <lakot/interface/graphics/render/IRenderer.h>

namespace lakot {

class ARenderer : public IRenderer
{
public:
    virtual ~ARenderer() override;
    ARenderer();
};

}

#endif
