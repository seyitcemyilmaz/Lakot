#ifndef LAKOT_PARTICLERENDERER_H
#define LAKOT_PARTICLERENDERER_H

#include <core/helper/shader/ShaderProgram.h>
#include <core/helper/camera/Camera.h>

#include "Particle.h"

class ParticleRenderer
{
private:
    unsigned int mVAO;
    unsigned int mVBO;
    unsigned int mIBO;

public:
    virtual ~ParticleRenderer();
    ParticleRenderer();

    void init();

    void render(ShaderProgram* pShaderProgram, Camera* pCamera, Particle* pParticle);
};

#endif
