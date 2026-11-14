#ifndef LAKOT_EFFECT_SYSTEM_H
#define LAKOT_EFFECT_SYSTEM_H

#include <cstdint>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "EffectCatalog.h"
#include "RollingAverage.hpp"

namespace lakot
{

class WorldEntities;

using EffectHandle = uint32_t;

struct Particle
{
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    float age{0.0f};
    float life{1.0f};
    float rotation{0.0f};
    float spin{0.0f};
    const EmitterDefinition* definition{nullptr};
};

struct EffectStats
{
    uint32_t emitterCount{0};
    uint32_t particleCount{0};
    float updateMs{0.0f};
};

class EffectSystem
{
public:
    static constexpr uint32_t kMaxEmitters = 512;
    static constexpr uint32_t kMaxParticles = 8192;

    explicit EffectSystem(EffectCatalog pCatalog);

    bool reload(const std::string& pPath, std::string& pError);

    EffectHandle spawn(std::string_view pName, const glm::vec3& pPosition);
    EffectHandle attach(std::string_view pName, uint64_t pEntityId, const glm::vec3& pOffset);
    void stop(EffectHandle pHandle);
    void clear();

    void update(float pDeltaSeconds, const WorldEntities& pEntities);

    const EffectCatalog& getCatalog() const;
    const std::vector<Particle>& getParticles() const;
    EffectStats getStats() const;

private:
    struct Emitter
    {
        const EmitterDefinition* definition{nullptr};
        glm::vec3 position{0.0f};
        glm::vec3 attachOffset{0.0f};
        uint64_t entityId{0};
        float age{0.0f};
        float spawnAccumulator{0.0f};
        bool isBurstDone{false};
        bool isStopped{false};
        EffectHandle handle{0};
    };

    EffectCatalog mCatalog;
    std::vector<Emitter> mEmitters;
    std::vector<Particle> mParticles;
    std::minstd_rand mRandom;
    EffectHandle mNextHandle{1};
    bool mIsParticleLimitLogged{false};
    std::set<std::string, std::less<>> mUnknownLogged;

    RollingAverage<120> mUpdateTime;

    EffectHandle addEmitters(std::string_view pName, const glm::vec3& pPosition, uint64_t pEntityId, const glm::vec3& pOffset);
    void updateEmitters(float pDeltaSeconds, const WorldEntities& pEntities);
    void updateParticles(float pDeltaSeconds);
    void emit(const Emitter& pEmitter, uint32_t pCount);

    float randomRange(float pMin, float pMax);
    glm::vec3 sampleShape(const EmitterDefinition& pDefinition);
    glm::vec3 sampleDirection(const EmitterDefinition& pDefinition);
};

}

#endif
