#include "WorldPicker.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "WorldEntities.h"
#include "WorldViewConstants.h"

#include "../../graphics/camera/Camera.h"

using namespace lakot;

namespace
{
    const glm::vec3 kGroundItemHalfExtents(0.5f, 0.4f, 0.5f);

    std::optional<float> intersectRayBox(const glm::vec3& pOrigin, const glm::vec3& pDirection,
                                         const glm::vec3& pBoxMin, const glm::vec3& pBoxMax)
    {
        float tEntry = 0.0f;
        float tExit = std::numeric_limits<float>::max();

        for (int tAxis = 0; tAxis < 3; ++tAxis)
        {
            if (std::fabs(pDirection[tAxis]) < 1e-8f)
            {
                if (pOrigin[tAxis] < pBoxMin[tAxis] || pOrigin[tAxis] > pBoxMax[tAxis])
                {
                    return std::nullopt;
                }

                continue;
            }

            float tInvDirection = 1.0f / pDirection[tAxis];
            float tSlabEntry = (pBoxMin[tAxis] - pOrigin[tAxis]) * tInvDirection;
            float tSlabExit = (pBoxMax[tAxis] - pOrigin[tAxis]) * tInvDirection;

            if (tSlabEntry > tSlabExit)
            {
                std::swap(tSlabEntry, tSlabExit);
            }

            tEntry = std::max(tEntry, tSlabEntry);
            tExit = std::min(tExit, tSlabExit);

            if (tEntry > tExit)
            {
                return std::nullopt;
            }
        }

        return tEntry;
    }
}

WorldPicker::WorldPicker(SDL_Window* pWindow, const Camera& pCamera, const WorldEntities& pEntities)
    : mWindow(pWindow)
    , mCamera(pCamera)
    , mEntities(pEntities)
{

}

glm::vec3 WorldPicker::getObjectCenter(const MapData& pMap, const MapObject& pObject)
{
    float tGround = pMap.getHeightAt(pObject.x, pObject.z);
    return glm::vec3(pObject.x, tGround + pObject.elevation + pObject.sizeY * 0.5f, pObject.z);
}

std::optional<WorldPicker::Ray> WorldPicker::makeRay(const glm::vec2& pWindowPixel) const
{
    int tLogicalWidth = 1;
    int tLogicalHeight = 1;
    SDL_GetWindowSize(mWindow, &tLogicalWidth, &tLogicalHeight);

    if (tLogicalWidth <= 0 || tLogicalHeight <= 0)
    {
        return std::nullopt;
    }

    float tNdcX = (pWindowPixel.x / static_cast<float>(tLogicalWidth)) * 2.0f - 1.0f;
    float tNdcY = 1.0f - (pWindowPixel.y / static_cast<float>(tLogicalHeight)) * 2.0f;

    glm::mat4 tInverseViewProjection = glm::inverse(mCamera.getViewProjectionMatrix());

    glm::vec4 tNearPoint = tInverseViewProjection * glm::vec4(tNdcX, tNdcY, -1.0f, 1.0f);
    glm::vec4 tFarPoint = tInverseViewProjection * glm::vec4(tNdcX, tNdcY, 1.0f, 1.0f);

    if (std::fabs(tNearPoint.w) < 1e-6f || std::fabs(tFarPoint.w) < 1e-6f)
    {
        return std::nullopt;
    }

    glm::vec3 tRayOrigin = glm::vec3(tNearPoint) / tNearPoint.w;
    glm::vec3 tRayEnd = glm::vec3(tFarPoint) / tFarPoint.w;
    glm::vec3 tRayDirection = tRayEnd - tRayOrigin;

    if (glm::length(tRayDirection) < 1e-8f)
    {
        return std::nullopt;
    }

    return Ray{ tRayOrigin, glm::normalize(tRayDirection) };
}

std::optional<glm::vec3> WorldPicker::pickGround(const MapData& pMap, const glm::vec2& pWindowPixel) const
{
    auto tRay = makeRay(pWindowPixel);

    if (!tRay)
    {
        return std::nullopt;
    }

    constexpr float kMarchStep = 0.5f;
    constexpr float kMaxDistance = WorldView::kViewDistance + 40.0f;

    auto isBelowGround = [&pMap, &tRay](float pDistance)
    {
        glm::vec3 tPoint = tRay->origin + tRay->direction * pDistance;
        return tPoint.y <= pMap.getHeightAt(tPoint.x, tPoint.z);
    };

    std::optional<float> tHitDistance;
    float tAbove = 0.0f;

    for (float tDistance = kMarchStep; tDistance <= kMaxDistance; tDistance += kMarchStep)
    {
        if (!isBelowGround(tDistance))
        {
            tAbove = tDistance;
            continue;
        }

        float tBelow = tDistance;

        for (int tIteration = 0; tIteration < 16; ++tIteration)
        {
            float tMiddle = (tAbove + tBelow) * 0.5f;
            (isBelowGround(tMiddle) ? tBelow : tAbove) = tMiddle;
        }

        tHitDistance = tBelow;
        break;
    }

    for (const MapObject& tObject : pMap.getObjects())
    {
        glm::vec3 tCenter = getObjectCenter(pMap, tObject);
        glm::vec3 tHalf(tObject.sizeX * 0.5f, tObject.sizeY * 0.5f, tObject.sizeZ * 0.5f);

        auto tEntry = intersectRayBox(tRay->origin, tRay->direction, tCenter - tHalf, tCenter + tHalf);

        if (tEntry && (!tHitDistance || *tEntry < *tHitDistance))
        {
            tHitDistance = *tEntry;
        }
    }

    if (!tHitDistance)
    {
        return std::nullopt;
    }

    constexpr float kBackOffStep = 0.5f;
    constexpr float kMaxBackOff = 40.0f;

    for (float tBack = 0.0f; tBack <= kMaxBackOff && *tHitDistance - tBack >= 0.0f; tBack += kBackOffStep)
    {
        glm::vec3 tPoint = tRay->origin + tRay->direction * (*tHitDistance - tBack);

        if (pMap.isWalkable(tPoint.x, tPoint.z))
        {
            return glm::vec3(tPoint.x, pMap.getHeightAt(tPoint.x, tPoint.z), tPoint.z);
        }
    }

    glm::vec3 tHit = tRay->origin + tRay->direction * *tHitDistance;
    return glm::vec3(tHit.x, pMap.getHeightAt(tHit.x, tHit.z), tHit.z);
}

std::optional<WorldPicker::Target> WorldPicker::pickTarget(const glm::vec2& pWindowPixel) const
{
    auto tRay = makeRay(pWindowPixel);

    if (!tRay)
    {
        return std::nullopt;
    }

    const WorldEntity* tClosest = nullptr;
    float tClosestDistance = std::numeric_limits<float>::max();

    for (const auto& [tId, tEntity] : mEntities.getAll())
    {
        if (tId == WorldEntities::kLocalId)
        {
            continue;
        }

        glm::vec3 tCenter = WorldEntities::getPosition(tEntity);
        glm::vec3 tHalf = WorldView::kPlayerBoxHalfExtents * tEntity.scale;
        tCenter.y += WorldView::kPlayerBoxHalfExtents.y * (tEntity.scale - 1.0f);

        if (tEntity.type == EntityType::eGroundItem)
        {
            tHalf = kGroundItemHalfExtents;
            tCenter.y += kGroundItemHalfExtents.y;
        }

        auto tEntry = intersectRayBox(tRay->origin, tRay->direction, tCenter - tHalf, tCenter + tHalf);

        if (tEntry && *tEntry < tClosestDistance)
        {
            tClosestDistance = *tEntry;
            tClosest = &tEntity;
        }
    }

    if (!tClosest)
    {
        return std::nullopt;
    }

    std::string tName = !tClosest->name.empty() ? tClosest->name : ("Player" + std::to_string(tClosest->id));

    return Target{ tClosest->type, tClosest->id, tName };
}
