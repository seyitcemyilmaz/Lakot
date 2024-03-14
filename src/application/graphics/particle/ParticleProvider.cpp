#include "ParticleProvider.h"

ParticleProvider::~ParticleProvider()
{

}

ParticleProvider::ParticleProvider(const glm::vec3& pPosition)
    : mPosition(pPosition)
    , mMaximumParticleCount(1000)
    , mLastUsedParticleIndex(0)
{
    mParticles.resize(mMaximumParticleCount);
}

void ParticleProvider::update(double pTimeDifference)
{
    for (unsigned int i = 0; i < mParticles.size(); i++)
    {
        mParticles[i]->update(pTimeDifference);
    }
}

void ParticleProvider::render(ShaderProgram* pShaderProgram, Camera* pCamera)
{
    for (unsigned int i = 0; i < mParticles.size(); i++)
    {
        // pShader->getShaderVariable(pCamera.)
    }
}

const glm::vec3& ParticleProvider::getPosition() const
{
    return mPosition;
}

unsigned int ParticleProvider::getMaximumParticleCount() const
{
    return mMaximumParticleCount;
}

void ParticleProvider::setMaximumParticleCount(unsigned int pMaximumParticleCount)
{
    mMaximumParticleCount = pMaximumParticleCount;
}

void ParticleProvider::setPosition(const glm::vec3& pPosition)
{
    mPosition = pPosition;
}

int ParticleProvider::findUnusedParticle()
{
    for (unsigned int i = mLastUsedParticleIndex; i < mMaximumParticleCount; i++)
    {
        if (!mParticles[i]->getIsActive())
        {
            mLastUsedParticleIndex = i;
            return static_cast<unsigned int>(i);
        }
    }

    for (unsigned int i = 0; i < mLastUsedParticleIndex; i++)
    {
        if (!mParticles[i]->getIsActive())
        {
            mLastUsedParticleIndex = i;
            return static_cast<unsigned int>(i);
        }
    }

    return -1;
}
