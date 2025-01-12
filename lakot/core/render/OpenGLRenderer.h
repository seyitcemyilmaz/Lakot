#ifndef LAKOT_OPENGLRENDERER_H
#define LAKOT_OPENGLRENDERER_H

#include <lakot/abstract/render/ARenderer.h>

#include "../graphics/shader/OpenGLShaderProgram.h"

namespace lakot {

class OpenGLRenderer : public ARenderer
{
public:
    virtual ~OpenGLRenderer() override;
    OpenGLRenderer();

    void initialize() override;
    void deinitialize() override;

    void render(ARenderable* pRenderable) override;

private:
    std::map<std::string, OpenGLShaderProgram*> mShaderPrograms;


};

}

#endif
