#ifndef LAKOT_OPENGLESRENDERER_H
#define LAKOT_OPENGLESRENDERER_H

#include "Renderer.h"

namespace lakot {

class OpenGLESRenderer : public Renderer
{
public:
    virtual ~OpenGLESRenderer() override;
    OpenGLESRenderer();

    void initialize() override;
    void deinitialize() override;

    void render(IRenderable* pRenderable) override;
};

}

#endif
