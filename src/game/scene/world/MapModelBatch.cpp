#include "MapModelBatch.h"

#include "WorldViewConstants.h"
#include "../../graphics/model/Model.h"

#include "../../graphics/camera/Frustum.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

using namespace lakot;

namespace
{
    constexpr float kLodDistance = 60.0f;
    constexpr float kRebuildDistance = 4.0f;
    constexpr float kFadeBandRatio = 0.3f;
    constexpr float kMinFadeScale = 0.05f;
    constexpr float kCellSize = 32.0f;
    constexpr float kModelRadius = 16.0f;
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
    mAreCellsDirty = true;
}

void MapModelBatch::buildCells()
{
    mCells.clear();
    mVisibleCells.clear();

    std::unordered_map<int64_t, uint32_t> tLookup;

    for (uint32_t tIndex = 0; tIndex < mOffsets.size(); ++tIndex)
    {
        const glm::vec3& tOffset = mOffsets[tIndex];
        const int64_t tCellX = static_cast<int64_t>(std::floor(tOffset.x / kCellSize));
        const int64_t tCellZ = static_cast<int64_t>(std::floor(tOffset.z / kCellSize));
        const int64_t tKey = (tCellX << 32) ^ (tCellZ & 0xffffffffLL);
        auto [tIt, tIsNew] = tLookup.try_emplace(tKey, static_cast<uint32_t>(mCells.size()));

        if (tIsNew)
        {
            Cell tCell;
            tCell.min = tOffset;
            tCell.max = tOffset;
            mCells.push_back(std::move(tCell));
        }

        Cell& tCell = mCells[tIt->second];
        tCell.indices.push_back(tIndex);
        tCell.min = glm::min(tCell.min, tOffset);
        tCell.max = glm::max(tCell.max, tOffset);
        tCell.maxExtent = std::max(tCell.maxExtent, mParams[tIndex].y * std::max(1.0f, mParams[tIndex].z));
    }

    mAreCellsDirty = false;
}

float MapModelBatch::getModelRadius() const
{
    float tRadius = mModel->getRadius();

    if (mLodModel)
    {
        tRadius = std::max(tRadius, mLodModel->getRadius());
    }

    return tRadius > 0.0f ? tRadius : kModelRadius;
}

float MapModelBatch::getCullDistance(float pViewDistance) const
{
    if (mDrawDistance > 0.0f)
    {
        return std::min(pViewDistance, mDrawDistance * pViewDistance / WorldView::kViewDistance);
    }

    return pViewDistance;
}

void MapModelBatch::collectVisibleCells(const glm::vec3& pCamera, float pCullDistance, const Frustum& pFrustum, std::vector<uint32_t>& pOut) const
{
    const float tCullSquared = pCullDistance * pCullDistance;

    for (uint32_t tIndex = 0; tIndex < mCells.size(); ++tIndex)
    {
        const Cell& tCell = mCells[tIndex];
        const float tDx = std::max({tCell.min.x - pCamera.x, 0.0f, pCamera.x - tCell.max.x});
        const float tDz = std::max({tCell.min.z - pCamera.z, 0.0f, pCamera.z - tCell.max.z});

        if (tDx * tDx + tDz * tDz > tCullSquared)
        {
            continue;
        }

        const float tMargin = getModelRadius() * std::max(1.0f, tCell.maxExtent);
        const glm::vec3 tPad(tMargin, tMargin, tMargin);

        if (pFrustum.intersectsBox(tCell.min - tPad, tCell.max + tPad))
        {
            pOut.push_back(tIndex);
        }
    }
}

void MapModelBatch::update(const glm::vec3& pCamera, float pViewDistance, const Frustum& pFrustum)
{
    if (mAreCellsDirty)
    {
        buildCells();
    }

    mScratchCells.clear();
    collectVisibleCells(pCamera, getCullDistance(pViewDistance), pFrustum, mScratchCells);

    const bool tIsMoved = glm::distance(pCamera, mLastCamera) > kRebuildDistance;

    if (mIsDirty || pViewDistance != mLastViewDistance || tIsMoved || mScratchCells != mVisibleCells)
    {
        mVisibleCells.swap(mScratchCells);
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
    const float tCullDistance = getCullDistance(pViewDistance);
    const float tFadeBand = tCullDistance * kFadeBandRatio;

    for (uint32_t tCellIndex : mVisibleCells)
    {
        for (uint32_t tIndex : mCells[tCellIndex].indices)
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
