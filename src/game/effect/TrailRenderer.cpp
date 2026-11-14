#include "TrailRenderer.h"

#include <algorithm>

#include <glad/glad.h>

#include "WeaponTrail.h"
#include "../graphics/camera/Camera.h"
#include "../graphics/render/ShaderManager.h"
#include "../graphics/render/ShaderProgram.h"

using namespace lakot;

namespace
{
    constexpr const char* kProgramName = "world_trail";
    constexpr size_t kInitialCapacity = 256;

    const char* const kVertexSource = R"(
        layout (location = 0) in vec4 aPositionAlpha;

        uniform mat4 uViewProjection;

        out float vAlpha;

        void main()
        {
            vAlpha = aPositionAlpha.w;
            gl_Position = uViewProjection * vec4(aPositionAlpha.xyz, 1.0);
        }
    )";

    const char* const kFragmentSource = R"(
        in float vAlpha;
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(vec3(1.0, 0.9, 0.7) * vAlpha, vAlpha * 0.35);
        }
    )";
}

TrailRenderer::TrailRenderer(ShaderManager& pShaders)
    : mShaders(pShaders)
{
    if (!mShaders.hasProgram(kProgramName))
    {
        mShaders.createProgram(kProgramName, kVertexSource, kFragmentSource);
    }

    glGenVertexArrays(1, &mVertexArray);
    glGenBuffers(1, &mBuffer);

    glBindVertexArray(mVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

TrailRenderer::~TrailRenderer()
{
    glDeleteBuffers(1, &mBuffer);
    glDeleteVertexArrays(1, &mVertexArray);
}

void TrailRenderer::render(const WeaponTrail& pTrail, const Camera& pCamera)
{
    ShaderProgram* tProgram = mShaders.hasProgram(kProgramName) ? mShaders.getProgram(kProgramName) : nullptr;

    if (!tProgram)
    {
        return;
    }

    mVertices.clear();
    mRuns.clear();

    for (const auto& [tId, tTrack] : pTrail.getTracks())
    {
        if (tTrack.samples.size() < 2)
        {
            continue;
        }

        for (const TrailSample& tSample : tTrack.samples)
        {
            float tAlpha = 1.0f - tSample.age / WeaponTrail::kLifetimeSeconds;
            mVertices.emplace_back(tSample.tip, tAlpha);
            mVertices.emplace_back(tSample.base, tAlpha);
        }

        mRuns.push_back(tTrack.samples.size() * 2);
    }

    if (mVertices.empty())
    {
        return;
    }

    glBindVertexArray(mVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, mBuffer);

    if (mVertices.size() > mCapacity)
    {
        mCapacity = std::max(mVertices.size(), kInitialCapacity);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mCapacity * sizeof(glm::vec4)), nullptr, GL_DYNAMIC_DRAW);
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(mVertices.size() * sizeof(glm::vec4)), mVertices.data());

    tProgram->bind();
    tProgram->setMat4("uViewProjection", pCamera.getViewProjectionMatrix());

    bool tWasCulling = glIsEnabled(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    GLint tFirst = 0;

    for (size_t tCount : mRuns)
    {
        glDrawArrays(GL_TRIANGLE_STRIP, tFirst, static_cast<GLsizei>(tCount));
        tFirst += static_cast<GLint>(tCount);
    }

    if (tWasCulling)
    {
        glEnable(GL_CULL_FACE);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    tProgram->unbind();

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
