#include "ParticleManager.h"

ParticleManager* ParticleManager::mInstance = nullptr;

ParticleManager* ParticleManager::getInstance()
{
    if (!mInstance)
    {
        mInstance = new ParticleManager();
    }

    return mInstance;
}

ParticleManager::ParticleManager()
    : mParticleRenderer(nullptr)
{

}

void ParticleManager::init()
{
    mParticleRenderer = new ParticleRenderer();
    mParticleRenderer->init();
}

void ParticleManager::update(double pTimeDifference)
{
    for (unsigned int i = 0; i < mParticleProviders.size(); i++)
    {
        mParticleProviders[i]->update(pTimeDifference);
    }
}

void ParticleManager::render(ShaderProgram* pShaderProgram, Camera* pCamera)
{
    for (unsigned i = 0; i < mParticleProviders.size(); i++)
    {
        mParticleProviders[i]->render(pShaderProgram, pCamera);
    }
}

void ParticleManager::addParticleProvider(ParticleProvider* pParticleProvider)
{
    if (!pParticleProvider)
    {
        return;
    }

    mParticleProviders.push_back(pParticleProvider);
}

const std::vector<ParticleProvider*>& ParticleManager::getParticleProviders() const
{
    return mParticleProviders;
}
