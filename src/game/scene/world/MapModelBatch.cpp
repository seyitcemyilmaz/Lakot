#include "MapModelBatch.h"

#include "WorldViewConstants.h"
#include "../../graphics/model/Model.h"

#include <algorithm>

using namespace lakot;

namespace
{
    constexpr float kLodDistance = 60.0f;
    constexpr float kRebuildDistance = 4.0f;
    constexpr float kFadeBandRatio = 0.3f;
    constexpr float kMinFadeScale = 0.05f;
}

MapModelBatch::MapModelBatch(std::shared_ptr<Model> pModel, std::shared_ptr<Model> pLodModel)
    : mModel(std::move(pModel))
    , mLodModel(std::move(pLodModel))
{
}

void MapModelBatch::addInstance(const glm::vec3& pOffset, const glm::vec3& pParams)
{
    mOffsets.push_back(pOffset);
    mParams.push_back(pParams);
    mIsDirty = true;
}

void MapModelBatch::update(const glm::vec3& pCamera, float pViewDistance)
{
    if (mIsDirty || pViewDistance != mLastViewDistance || glm::distance(pCamera, mLastCamera) > kRebuildDistance)
    {
        rebuild(pCamera, pViewDistance);
    }
}

void MapModelBatch::rebuild(const glm::vec3& pCamera, float pViewDistance)
{
    mLastViewDistance = pViewDistance;
    std::vector<glm::vec3> tNearOffsets;
    std::vector<glm::vec3> tNearParams;
    std::vector<glm::vec3> tFarOffsets;
    std::vector<glm::vec3> tFarParams;

    const bool tIsLimited = mDrawDistance > 0.0f;
    const float tCullDistance = tIsLimited ? std::min(pViewDistance, mDrawDistance * pViewDistance / WorldView::kViewDistance) : pViewDistance;
    const float tFadeBand = tCullDistance * kFadeBandRatio;

    for (size_t tIndex = 0; tIndex < mOffsets.size(); ++tIndex)
    {
        float tDistance = glm::distance(glm::vec2(mOffsets[tIndex].x, mOffsets[tIndex].z), glm::vec2(pCamera.x, pCamera.z));

        if (tDistance > tCullDistance)
        {
            continue;
        }

        glm::vec3 tParams = mParams[tIndex];

        if (tIsLimited)
        {
            float tFade = std::clamp((tCullDistance - tDistance) / tFadeBand, 0.0f, 1.0f);

            if (tFade < kMinFadeScale)
            {
                continue;
            }

            tParams.y *= tFade;
        }

        bool tIsFar = mLodModel && tDistance > kLodDistance;
        (tIsFar ? tFarOffsets : tNearOffsets).push_back(mOffsets[tIndex]);
        (tIsFar ? tFarParams : tNearParams).push_back(tParams);
    }

    mModel->setInstances(tNearOffsets, tNearParams);

    if (mLodModel)
    {
        mLodModel->setInstances(tFarOffsets, tFarParams);
    }

    mLastCamera = pCamera;
    mIsDirty = false;
}

void MapModelBatch::draw(unsigned int pTextureUnit) const
{
    mModel->draw(pTextureUnit);

    if (mLodModel)
    {
        mLodModel->draw(pTextureUnit);
    }
}
