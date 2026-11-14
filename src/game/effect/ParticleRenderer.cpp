#include "ParticleRenderer.h"

#include <algorithm>

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>
#include <glad/glad.h>

#include "../asset/AssetManager.h"
#include "../graphics/camera/Camera.h"
#include "../graphics/render/ShaderManager.h"
#include "../graphics/render/ShaderProgram.h"

using namespace lakot;

namespace
{
    constexpr const char* kProgramName = "world_particle";
    constexpr float kMaxExtraSeconds = 0.1f;

    const char* const kVertexSource = R"(
        layout (location = 0) in vec2 aCorner;
        layout (location = 1) in vec4 aCenterSize;
        layout (location = 2) in vec4 aColor;
        layout (location = 3) in vec4 aVelocityStretch;
        layout (location = 4) in vec4 aParams;

        uniform mat4 uViewProjection;
        uniform vec3 uCameraRight;
        uniform vec3 uCameraUp;
        uniform vec3 uCameraPosition;

        out vec2 vUv;
        out vec4 vColor;
        out vec3 vWorldPosition;
        flat out float vLayer;

        void main()
        {
            vec3 tCenter = aCenterSize.xyz;
            float tSize = aCenterSize.w;
            int tOrientation = int(aParams.z + 0.5);
            float tCos = cos(aParams.x);
            float tSin = sin(aParams.x);
            vec2 tRotated = vec2(tCos * aCorner.x - tSin * aCorner.y, tSin * aCorner.x + tCos * aCorner.y);
            vec3 tWorld;

            if (tOrientation == 2)
            {
                tWorld = tCenter + vec3(0.0, 0.03, 0.0) + vec3(tRotated.x, 0.0, tRotated.y) * tSize;
            }
            else if (tOrientation == 1)
            {
                vec3 tView = normalize(uCameraPosition - tCenter);
                vec3 tVelocity = aVelocityStretch.xyz;
                vec3 tAxis = tVelocity - tView * dot(tVelocity, tView);

                if (length(tAxis) < 0.0001)
                {
                    tAxis = uCameraUp;
                }

                tAxis = normalize(tAxis);
                vec3 tSide = normalize(cross(tAxis, tView));
                float tLength = tSize * (1.0 + length(tVelocity) * aVelocityStretch.w);
                tWorld = tCenter + tSide * aCorner.x * tSize + tAxis * aCorner.y * tLength;
            }
            else
            {
                tWorld = tCenter + (uCameraRight * tRotated.x + uCameraUp * tRotated.y) * tSize;
            }

            vUv = aCorner + vec2(0.5);
            vColor = aColor;
            vWorldPosition = tWorld;
            vLayer = aParams.y;
            gl_Position = uViewProjection * vec4(tWorld, 1.0);
        }
    )";

    const char* const kFragmentSource = R"(
#ifdef GL_ES
        precision highp sampler2DArray;
#endif
        in vec2 vUv;
        in vec4 vColor;
        in vec3 vWorldPosition;
        flat in float vLayer;
        out vec4 FragColor;

        uniform sampler2DArray uTexture;
        uniform int uIsAdditive;
        uniform vec3 uCameraPosition;
        uniform vec3 uFogColor;
        uniform float uFogStart;
        uniform float uFogEnd;

        void main()
        {
            vec4 tColor = texture(uTexture, vec3(vUv, vLayer)) * vColor;
            float tFog = clamp((distance(vWorldPosition, uCameraPosition) - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);

            if (uIsAdditive == 1)
            {
                tColor.rgb *= (1.0 - tFog);
            }
            else
            {
                tColor.rgb = mix(tColor.rgb, uFogColor, tFog);
            }

            FragColor = tColor;
        }
    )";
}

ParticleRenderer::ParticleRenderer(ShaderManager& pShaders, AssetManager& pAssets)
    : mShaders(pShaders)
    , mAssets(pAssets)
{
    if (!mShaders.hasProgram(kProgramName))
    {
        mShaders.createProgram(kProgramName, kVertexSource, kFragmentSource);
    }

    mAlphaBatch.initialize();
    mAdditiveBatch.initialize();

#if defined(LAKOT_RENDERER_OPENGL_TYPE_CORE)
    glGenQueries(2, mQueries);
    mIsGpuTimed = true;
#endif
}

ParticleRenderer::~ParticleRenderer()
{
    if (mIsGpuTimed)
    {
        glDeleteQueries(2, mQueries);
    }
}

void ParticleRenderer::loadTextures(const EffectCatalog& pCatalog)
{
    std::vector<std::string> tPaths;

    for (const std::string& tName : pCatalog.getTextureNames())
    {
        tPaths.push_back(mAssets.resolve("effects/textures/" + tName + ".png"));
    }

    mTextures = TextureArray::fromFiles(tPaths);
}

void ParticleRenderer::render(const EffectSystem& pSystem, const Camera& pCamera, const glm::vec3& pFogColor, float pFogStart, float pFogEnd, double pExtraSeconds)
{
    const std::vector<Particle>& tParticles = pSystem.getParticles();
    ShaderProgram* tProgram = mShaders.hasProgram(kProgramName) ? mShaders.getProgram(kProgramName) : nullptr;

    if (tParticles.empty() || !tProgram || !mTextures)
    {
        return;
    }

    Uint64 tStart = SDL_GetPerformanceCounter();
    float tExtraSeconds = std::min(static_cast<float>(pExtraSeconds), kMaxExtraSeconds);
    const glm::vec3& tCameraPosition = pCamera.getPosition();

    mAlphaEntries.clear();
    mAdditiveEntries.clear();

    for (uint32_t tIndex = 0; tIndex < tParticles.size(); tIndex++)
    {
        const Particle& tParticle = tParticles[tIndex];
        glm::vec3 tOffset = tParticle.position - tCameraPosition;
        DrawEntry tEntry{ glm::dot(tOffset, tOffset), tIndex };

        if (tParticle.definition->blend == BlendType::eAlpha)
        {
            mAlphaEntries.push_back(tEntry);
        }
        else
        {
            mAdditiveEntries.push_back(tEntry);
        }
    }

    std::sort(mAlphaEntries.begin(), mAlphaEntries.end(), [](const DrawEntry& pFirst, const DrawEntry& pSecond)
    {
        return pFirst.distanceSquared > pSecond.distanceSquared;
    });

    fillBatch(mAlphaBatch, mAlphaEntries, tParticles, tExtraSeconds);
    fillBatch(mAdditiveBatch, mAdditiveEntries, tParticles, tExtraSeconds);

    beginGpuTimer();

    tProgram->bind();
    tProgram->setMat4("uViewProjection", pCamera.getViewProjectionMatrix());
    tProgram->setVec3("uCameraRight", pCamera.getRightVector());
    tProgram->setVec3("uCameraUp", pCamera.getUpVector());
    tProgram->setVec3("uCameraPosition", tCameraPosition);
    tProgram->setVec3("uFogColor", pFogColor);
    tProgram->setFloat("uFogStart", pFogStart);
    tProgram->setFloat("uFogEnd", pFogEnd);
    tProgram->setInt("uTexture", 0);
    mTextures->bind(0);

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    tProgram->setInt("uIsAdditive", 0);
    draw(mAlphaBatch);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    tProgram->setInt("uIsAdditive", 1);
    draw(mAdditiveBatch);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    tProgram->unbind();

    endGpuTimer();

    double tElapsed = static_cast<double>(SDL_GetPerformanceCounter() - tStart) * 1000.0 / static_cast<double>(SDL_GetPerformanceFrequency());
    mBuildTime.add(static_cast<float>(tElapsed));
}

void ParticleRenderer::fillBatch(ParticleBatch& pBatch, const std::vector<DrawEntry>& pEntries, const std::vector<Particle>& pParticles, float pExtraSeconds)
{
    mCenterSize.clear();
    mColor.clear();
    mVelocityStretch.clear();
    mParams.clear();

    for (const DrawEntry& tEntry : pEntries)
    {
        const Particle& tParticle = pParticles[tEntry.particleIndex];
        const EmitterDefinition& tDefinition = *tParticle.definition;
        float tProgress = tParticle.age / tParticle.life;
        glm::vec3 tPosition = tParticle.position + tParticle.velocity * pExtraSeconds;

        mCenterSize.emplace_back(tPosition, tDefinition.size.evaluate(tProgress));
        mColor.push_back(tDefinition.color.evaluate(tProgress));
        mVelocityStretch.emplace_back(tParticle.velocity, tDefinition.stretch);
        mParams.emplace_back(tParticle.rotation + tParticle.spin * pExtraSeconds, static_cast<float>(tDefinition.textureIndex),
                             static_cast<float>(tDefinition.orientation), 0.0f);
    }

    pBatch.setInstances(mCenterSize, mColor, mVelocityStretch, mParams);
}

void ParticleRenderer::draw(ParticleBatch& pBatch)
{
    unsigned int tCount = pBatch.getInstanceCount();

    if (tCount == 0)
    {
        return;
    }

    pBatch.getVertexArrayObject().bind();
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, tCount);
}

void ParticleRenderer::beginGpuTimer()
{
#if defined(LAKOT_RENDERER_OPENGL_TYPE_CORE)
    mQueryIndex = 1 - mQueryIndex;
    glBeginQuery(GL_TIME_ELAPSED, mQueries[mQueryIndex]);
#endif
}

void ParticleRenderer::endGpuTimer()
{
#if defined(LAKOT_RENDERER_OPENGL_TYPE_CORE)
    glEndQuery(GL_TIME_ELAPSED);
    mIsQueryIssued[mQueryIndex] = true;

    unsigned int tOther = 1 - mQueryIndex;

    if (!mIsQueryIssued[tOther])
    {
        return;
    }

    GLint tIsAvailable = 0;
    glGetQueryObjectiv(mQueries[tOther], GL_QUERY_RESULT_AVAILABLE, &tIsAvailable);

    if (tIsAvailable)
    {
        GLuint64 tNanoseconds = 0;
        glGetQueryObjectui64v(mQueries[tOther], GL_QUERY_RESULT, &tNanoseconds);
        mGpuTime.add(static_cast<float>(tNanoseconds) / 1.0e6f);
        mIsQueryIssued[tOther] = false;
    }
#endif
}

ParticleRenderStats ParticleRenderer::getStats() const
{
    ParticleRenderStats tStats;
    tStats.buildMs = mBuildTime.get();
    tStats.gpuMs = mIsGpuTimed ? mGpuTime.get() : -1.0f;
    return tStats;
}
