#ifndef LAKOT_OPENGLESRENDERER_H
#define LAKOT_OPENGLESRENDERER_H

#include <lakot/abstract/render/ARenderer.h>

#include "../graphics/shader/OpenGLShaderProgram.h"

namespace lakot {

class OpenGLESRenderer : public ARenderer
{
public:
    virtual ~OpenGLESRenderer() override;
    OpenGLESRenderer();

    void initialize() override;
    void deinitialize() override;

    void render(ARenderable* pRenderable) override;

private:
    std::map<std::string, OpenGLShaderProgram*> mShaderPrograms;
};

}

#endif
