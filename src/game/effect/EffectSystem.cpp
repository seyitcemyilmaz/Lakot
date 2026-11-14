#include "EffectSystem.h"

#include <algorithm>
#include <cmath>

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>
#include <glm/gtc/constants.hpp>

#include "../scene/world/WorldEntities.h"

using namespace lakot;

EffectSystem::EffectSystem(EffectCatalog pCatalog)
    : mCatalog(std::move(pCatalog))
    , mRandom(std::random_device{}())
{
    mEmitters.reserve(kMaxEmitters);
    mParticles.reserve(kMaxParticles);
}

bool EffectSystem::reload(const std::string& pPath, std::string& pError)
{
    EffectCatalog tCatalog;

    if (!tCatalog.load(pPath, pError))
    {
        return false;
    }

    clear();
    mCatalog = std::move(tCatalog);
    mUnknownLogged.clear();
    return true;
}

EffectHandle EffectSystem::spawn(std::string_view pName, const glm::vec3& pPosition)
{
    return addEmitters(pName, pPosition, 0, glm::vec3(0.0f));
}

EffectHandle EffectSystem::attach(std::string_view pName, uint64_t pEntityId, const glm::vec3& pOffset)
{
    return addEmitters(pName, glm::vec3(0.0f), pEntityId, pOffset);
}

EffectHandle EffectSystem::addEmitters(std::string_view pName, const glm::vec3& pPosition, uint64_t pEntityId, const glm::vec3& pOffset)
{
    const EffectDefinition* tEffect = mCatalog.find(pName);

    if (!tEffect)
    {
        if (mUnknownLogged.emplace(pName).second)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "EffectSystem: unknown effect %.*s", static_cast<int>(pName.size()), pName.data());
        }

        return 0;
    }

    if (mEmitters.size() + tEffect->emitters.size() > kMaxEmitters)
    {
        return 0;
    }

    EffectHandle tHandle = mNextHandle++;

    for (const EmitterDefinition& tDefinition : tEffect->emitters)
    {
        Emitter tEmitter;
        tEmitter.definition = &tDefinition;
        tEmitter.position = pPosition;
        tEmitter.attachOffset = pOffset;
        tEmitter.entityId = pEntityId;
        tEmitter.handle = tHandle;
        mEmitters.push_back(tEmitter);
    }

    return tHandle;
}

void EffectSystem::stop(EffectHandle pHandle)
{
    if (pHandle == 0)
    {
        return;
    }

    for (Emitter& tEmitter : mEmitters)
    {
        if (tEmitter.handle == pHandle)
        {
            tEmitter.isStopped = true;
        }
    }
}

void EffectSystem::clear()
{
    mEmitters.clear();
    mParticles.clear();
}

void EffectSystem::update(float pDeltaSeconds, const WorldEntities& pEntities)
{
    Uint64 tStart = SDL_GetPerformanceCounter();

    updateEmitters(pDeltaSeconds, pEntities);
    updateParticles(pDeltaSeconds);

    double tElapsed = static_cast<double>(SDL_GetPerformanceCounter() - tStart) * 1000.0 / static_cast<double>(SDL_GetPerformanceFrequency());
    mUpdateTime.add(static_cast<float>(tElapsed));
}

void EffectSystem::updateEmitters(float pDeltaSeconds, const WorldEntities& pEntities)
{
    for (size_t tIndex = 0; tIndex < mEmitters.size();)
    {
        Emitter& tEmitter = mEmitters[tIndex];
        const EmitterDefinition& tDefinition = *tEmitter.definition;

        if (tEmitter.entityId != 0 && !tEmitter.isStopped)
        {
            if (auto tPosition = pEntities.findPosition(tEmitter.entityId))
            {
                tEmitter.position = *tPosition + tEmitter.attachOffset;
            }
            else
            {
                tEmitter.isStopped = true;
            }
        }

        if (!tEmitter.isStopped)
        {
            if (!tEmitter.isBurstDone)
            {
                tEmitter.isBurstDone = true;
                emit(tEmitter, tDefinition.burst);
            }

            tEmitter.age += pDeltaSeconds;

            if (tDefinition.rate > 0.0f && (tDefinition.isLoop || tEmitter.age <= tDefinition.duration))
            {
                tEmitter.spawnAccumulator += tDefinition.rate * pDeltaSeconds;
                uint32_t tCount = static_cast<uint32_t>(tEmitter.spawnAccumulator);
                tEmitter.spawnAccumulator -= static_cast<float>(tCount);
                emit(tEmitter, tCount);
            }
        }

        bool tIsFinished = tEmitter.isStopped || (!tDefinition.isLoop && tEmitter.age >= tDefinition.duration);

        if (tIsFinished)
        {
            tEmitter = mEmitters.back();
            mEmitters.pop_back();
        }
        else
        {
            tIndex++;
        }
    }
}

void EffectSystem::updateParticles(float pDeltaSeconds)
{
    for (size_t tIndex = 0; tIndex < mParticles.size();)
    {
        Particle& tParticle = mParticles[tIndex];
        tParticle.age += pDeltaSeconds;

        if (tParticle.age >= tParticle.life)
        {
            tParticle = mParticles.back();
            mParticles.pop_back();
            continue;
        }

        const EmitterDefinition& tDefinition = *tParticle.definition;
        tParticle.velocity.y += tDefinition.gravity * pDeltaSeconds;
        tParticle.velocity *= std::max(0.0f, 1.0f - tDefinition.drag * pDeltaSeconds);
        tParticle.position += tParticle.velocity * pDeltaSeconds;
        tParticle.rotation += tParticle.spin * pDeltaSeconds;
        tIndex++;
    }
}

void EffectSystem::emit(const Emitter& pEmitter, uint32_t pCount)
{
    const EmitterDefinition& tDefinition = *pEmitter.definition;

    for (uint32_t tIndex = 0; tIndex < pCount; tIndex++)
    {
        if (mParticles.size() >= kMaxParticles)
        {
            if (!mIsParticleLimitLogged)
            {
                mIsParticleLimitLogged = true;
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "EffectSystem: particle limit %u reached, dropping new particles", kMaxParticles);
            }

            return;
        }

        Particle tParticle;
        tParticle.definition = pEmitter.definition;
        tParticle.position = pEmitter.position + tDefinition.offset + sampleShape(tDefinition);
        tParticle.velocity = sampleDirection(tDefinition) * randomRange(tDefinition.speed.min, tDefinition.speed.max);
        tParticle.life = randomRange(tDefinition.lifetime.min, tDefinition.lifetime.max);
        tParticle.rotation = glm::radians(randomRange(tDefinition.rotationDegrees.min, tDefinition.rotationDegrees.max));
        tParticle.spin = glm::radians(randomRange(tDefinition.spinDegrees.min, tDefinition.spinDegrees.max));
        mParticles.push_back(tParticle);
    }
}

float EffectSystem::randomRange(float pMin, float pMax)
{
    if (pMax <= pMin)
    {
        return pMin;
    }

    return pMin + (pMax - pMin) * std::uniform_real_distribution<float>(0.0f, 1.0f)(mRandom);
}

glm::vec3 EffectSystem::sampleShape(const EmitterDefinition& pDefinition)
{
    if (pDefinition.shapeRadius <= 0.0f || pDefinition.shape == EmitterShapeType::ePoint)
    {
        return glm::vec3(0.0f);
    }

    float tAngle = randomRange(0.0f, glm::two_pi<float>());

    if (pDefinition.shape == EmitterShapeType::eRing)
    {
        return glm::vec3(std::cos(tAngle), 0.0f, std::sin(tAngle)) * pDefinition.shapeRadius;
    }

    float tHeight = randomRange(-1.0f, 1.0f);
    float tRing = std::sqrt(1.0f - tHeight * tHeight);
    float tRadius = std::cbrt(randomRange(0.0f, 1.0f)) * pDefinition.shapeRadius;
    return glm::vec3(std::cos(tAngle) * tRing, tHeight, std::sin(tAngle) * tRing) * tRadius;
}

glm::vec3 EffectSystem::sampleDirection(const EmitterDefinition& pDefinition)
{
    float tLength = glm::length(pDefinition.direction);
    glm::vec3 tAxis = tLength > 1e-4f ? pDefinition.direction / tLength : glm::vec3(0.0f, 1.0f, 0.0f);

    if (pDefinition.spreadDegrees <= 0.0f)
    {
        return tAxis;
    }

    float tCosMax = std::cos(glm::radians(pDefinition.spreadDegrees));
    float tCosTheta = 1.0f - randomRange(0.0f, 1.0f) * (1.0f - tCosMax);
    float tSinTheta = std::sqrt(std::max(0.0f, 1.0f - tCosTheta * tCosTheta));
    float tPhi = randomRange(0.0f, glm::two_pi<float>());

    glm::vec3 tHelper = std::abs(tAxis.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 tFirst = glm::normalize(glm::cross(tAxis, tHelper));
    glm::vec3 tSecond = glm::cross(tAxis, tFirst);

    return tAxis * tCosTheta + (tFirst * std::cos(tPhi) + tSecond * std::sin(tPhi)) * tSinTheta;
}

const EffectCatalog& EffectSystem::getCatalog() const
{
    return mCatalog;
}

const std::vector<Particle>& EffectSystem::getParticles() const
{
    return mParticles;
}

EffectStats EffectSystem::getStats() const
{
    EffectStats tStats;
    tStats.emitterCount = static_cast<uint32_t>(mEmitters.size());
    tStats.particleCount = static_cast<uint32_t>(mParticles.size());

    tStats.updateMs = mUpdateTime.get();
    return tStats;
}
