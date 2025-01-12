#include "ARenderer.h"

using namespace lakot;

ARenderer::~ARenderer()
{

}

ARenderer::ARenderer()
    : Object()
{

}

void ARenderer::setProjectionMatrix(const glm::mat4& pProjectionMatrix)
{
    mProjectionMatrix = pProjectionMatrix;
}

void ARenderer::setViewMatrix(const glm::mat4& pViewMatrix)
{
    mViewMatrix = pViewMatrix;
}
