#ifndef LAKOT_PARTICLEPROVIDER_H
#define LAKOT_PARTICLEPROVIDER_H

#include <vector>

#include <core/helper/camera/Camera.h>
#include <core/helper/shader/ShaderProgram.h>

#include "Particle.h"


class ParticleProvider
{
private:
    std::vector<Particle*> mParticles;

    glm::vec3 mPosition;

    unsigned int mMaximumParticleCount;

    unsigned int mLastUsedParticleIndex;

    int findUnusedParticle();

public:
    virtual ~ParticleProvider();
    ParticleProvider(const glm::vec3& pPosition = glm::vec3(0.0f, 0.0f, 0.0f));

    void update(double pTimeDifference);
    void render(ShaderProgram* pShaderProgram, Camera* pCamera);

    const glm::vec3& getPosition() const;
    unsigned int getMaximumParticleCount() const;

    void setPosition(const glm::vec3& pPosition);
    void setMaximumParticleCount(unsigned int pMaximumParticleCount);
};

#endif
