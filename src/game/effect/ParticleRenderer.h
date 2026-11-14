#ifndef LAKOT_PARTICLE_RENDERER_H
#define LAKOT_PARTICLE_RENDERER_H

#include <cstdint>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "EffectSystem.h"
#include "RollingAverage.hpp"
#include "../graphics/geometry/ParticleBatch.h"
#include "../graphics/model/TextureArray.h"

namespace lakot
{

class AssetManager;
class Camera;
class ShaderManager;

struct ParticleRenderStats
{
    float buildMs{0.0f};
    float gpuMs{-1.0f};
};

class ParticleRenderer
{
public:
    ParticleRenderer(ShaderManager& pShaders, AssetManager& pAssets);
    ~ParticleRenderer();

    ParticleRenderer(const ParticleRenderer&) = delete;
    ParticleRenderer& operator=(const ParticleRenderer&) = delete;

    void loadTextures(const EffectCatalog& pCatalog);

    void render(const EffectSystem& pSystem, const Camera& pCamera, const glm::vec3& pFogColor, float pFogStart, float pFogEnd, double pExtraSeconds);

    ParticleRenderStats getStats() const;

private:
    struct DrawEntry
    {
        float distanceSquared;
        uint32_t particleIndex;
    };

    ShaderManager& mShaders;
    AssetManager& mAssets;

    std::unique_ptr<TextureArray> mTextures;
    ParticleBatch mAlphaBatch;
    ParticleBatch mAdditiveBatch;

    std::vector<DrawEntry> mAlphaEntries;
    std::vector<DrawEntry> mAdditiveEntries;
    std::vector<glm::vec4> mCenterSize;
    std::vector<glm::vec4> mColor;
    std::vector<glm::vec4> mVelocityStretch;
    std::vector<glm::vec4> mParams;

    RollingAverage<120> mBuildTime;
    RollingAverage<120> mGpuTime;
    bool mIsGpuTimed{false};
    unsigned int mQueries[2]{0, 0};
    bool mIsQueryIssued[2]{false, false};
    unsigned int mQueryIndex{0};

    void fillBatch(ParticleBatch& pBatch, const std::vector<DrawEntry>& pEntries, const std::vector<Particle>& pParticles, float pExtraSeconds);
    void draw(ParticleBatch& pBatch);
    void beginGpuTimer();
    void endGpuTimer();
};

}

#endif
