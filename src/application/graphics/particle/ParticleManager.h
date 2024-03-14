#ifndef LAKOT_PARTICLEMANAGER_H
#define LAKOT_PARTICLEMANAGER_H

#include <core/helper/shader/Shader.h>

#include "ParticleProvider.h"
#include "ParticleRenderer.h"

class ParticleManager
{
private:
    static ParticleManager* mInstance;

    std::vector<ParticleProvider*> mParticleProviders;

    ParticleRenderer* mParticleRenderer;

    ParticleManager();

public:
    static ParticleManager* getInstance();

    void init();

    void update(double pTimeDifference);
    void render(ShaderProgram* pShaderProgram, Camera* pCamera);

    void addParticleProvider(ParticleProvider* pParticleProvider);

    const std::vector<ParticleProvider*>& getParticleProviders() const;
};

#endif
