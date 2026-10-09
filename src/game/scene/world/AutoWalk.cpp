#include "AutoWalk.h"

#include "Pathfinder.h"

#include "WorldEntities.h"

using namespace lakot;

namespace
{
    constexpr float kFollowStopDistance = 2.5f;

    constexpr float kFollowRepathDistance = 3.0f;
    constexpr double kRepathInterval = 0.5;
}

AutoWalk::AutoWalk(const WorldEntities& pEntities)
    : mEntities(pEntities)
{

}

void AutoWalk::walkTo(const MapData& pMap, const glm::vec2& pFrom, const glm::vec2& pGoal)
{
    stop();

    mGoal = pGoal;
    mPath = Pathfinder::findPath(pMap, pFrom, pGoal);
    mPathIndex = 0;
}

void AutoWalk::follow(uint64_t pPlayerId)
{
    stop();

    mIsFollowing = true;
    mFollowTargetId = pPlayerId;
    mRepathTimer = 0.0;
}

void AutoWalk::stop()
{
    mPath.clear();
    mPathIndex = 0;
    mIsFollowing = false;
    mFollowTargetId = 0;
}

bool AutoWalk::isActive() const
{
    return mIsFollowing || mPathIndex < mPath.size();
}

glm::vec2 AutoWalk::advance(const MapData& pMap, const glm::vec2& pPosition, float pDistance, double pDeltaTime)
{
    if (mIsFollowing)
    {
        auto tTarget3 = mEntities.findPosition(mFollowTargetId);

        if (!tTarget3)
        {
            stop();
            return pPosition;
        }

        glm::vec2 tTarget(tTarget3->x, tTarget3->z);

        if (glm::distance(pPosition, tTarget) <= kFollowStopDistance)
        {
            stop();
            return pPosition;
        }

        mRepathTimer -= pDeltaTime;

        bool tIsStale = mPathIndex >= mPath.size()
                        || glm::distance(tTarget, mFollowPathGoal) > kFollowRepathDistance;

        if (mRepathTimer <= 0.0 && tIsStale)
        {
            mPath = Pathfinder::findPath(pMap, pPosition, tTarget);
            mPathIndex = 0;
            mFollowPathGoal = tTarget;
            mRepathTimer = kRepathInterval;
        }
    }

    glm::vec2 tPosition = pPosition;
    float tRemaining = pDistance;

    while (tRemaining > 0.0f && mPathIndex < mPath.size())
    {
        const glm::vec2& tWaypoint = mPath[mPathIndex];
        float tToWaypoint = glm::distance(tPosition, tWaypoint);

        if (tToWaypoint <= tRemaining)
        {
            tPosition = tWaypoint;
            tRemaining -= tToWaypoint;
            ++mPathIndex;
        }
        else
        {
            tPosition += (tWaypoint - tPosition) * (tRemaining / tToWaypoint);
            tRemaining = 0.0f;
        }
    }

    if (!pMap.isMoveAllowed(pPosition.x, pPosition.y, tPosition.x, tPosition.y))
    {
        stop();
        return pPosition;
    }

    if (!mIsFollowing && mPathIndex >= mPath.size())
    {
        stop();
    }

    return tPosition;
}

void AutoWalk::onRepositioned(const MapData& pMap, const glm::vec2& pPosition)
{
    if (mIsFollowing)
    {
        mPath.clear();
        mRepathTimer = 0.0;
    }
    else if (!mPath.empty())
    {
        walkTo(pMap, pPosition, mGoal);
    }
}

std::optional<glm::vec2> AutoWalk::getDestination() const
{
    if (mIsFollowing || mPath.empty())
    {
        return std::nullopt;
    }

    return mPath.back();
}
