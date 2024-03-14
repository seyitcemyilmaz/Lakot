#include "ParticleRenderer.h"

#include <platform/Platform.h>

ParticleRenderer::~ParticleRenderer()
{

}

ParticleRenderer::ParticleRenderer()
    : mVAO(UINT_MAX)
    , mVBO(UINT_MAX)
    , mIBO(UINT_MAX)
{

}

void ParticleRenderer::init()
{
    unsigned int tIndices[] = { 0, 1, 2, 2, 3, 0 };

    float tVertices[] = {
        0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
        0.0f, 0.0f, 0.0f,   0.0f, 0.0f,

        0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        1.0f, 1.0f, 0.0f,   1.0f, 1.0f,
        1.0f, 0.0f, 0.0f,   1.0f, 0.0f
    };

    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);
    glGenBuffers(1, &mIBO);

    glBindVertexArray(mVAO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tIndices), tIndices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tVertices), tVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(tVertices), reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(tVertices), reinterpret_cast<const void*>(3 * sizeof(float)));

    glBindVertexArray(0);
}

void ParticleRenderer::render(ShaderProgram* pShaderProgram, Camera* pCamera, Particle* pParticle)
{
    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}
