#ifndef LAKOT_OPENGLRENDERER_H
#define LAKOT_OPENGLRENDERER_H

#include "Renderer.h"

namespace lakot {

class OpenGLRenderer : public Renderer
{
public:
    virtual ~OpenGLRenderer() override;
    OpenGLRenderer();

    void initialize() override;
    void deinitialize() override;

    void render(IRenderable* pRenderable) override;
};

}

#endif
