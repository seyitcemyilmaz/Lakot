#include "TargetController.h"

#include <cmath>

#include "AutoWalk.h"
#include "CombatRules.h"
#include "WorldEntities.h"
#include "WorldHud.h"

using namespace lakot;

namespace
{
    constexpr float kAttackReach = CombatRules::kPlayerAttackRange + 0.2f;
    constexpr float kDropDistance = 40.0f;
}

TargetController::TargetController(WorldEntities& pEntities, AutoWalk& pAutoWalk, WorldHud& pHud)
    : mEntities(pEntities)
    , mAutoWalk(pAutoWalk)
    , mHud(pHud)
{

}

void TargetController::select(uint64_t pEntityId)
{
    mTargetId = pEntityId;
    mIsInRange = false;
    mAutoWalk.follow(pEntityId);
}

void TargetController::clear()
{
    if (mTargetId == 0)
    {
        return;
    }

    mTargetId = 0;
    mIsInRange = false;
    mAutoWalk.stop();
    mHud.clearTarget();
}

void TargetController::update(const glm::vec3& pLocalPosition, bool pIsLocalDead)
{
    if (mTargetId == 0)
    {
        return;
    }

    auto tIterator = mEntities.getAll().find(mTargetId);

    if (pIsLocalDead || tIterator == mEntities.getAll().end() || tIterator->second.isDead)
    {
        clear();
        return;
    }

    const WorldEntity& tTarget = tIterator->second;
    glm::vec3 tPosition = WorldEntities::getPosition(tTarget);
    glm::vec2 tOffset(tPosition.x - pLocalPosition.x, tPosition.z - pLocalPosition.z);
    float tDistance = glm::length(tOffset);

    if (tDistance > kDropDistance)
    {
        clear();
        return;
    }

    mHud.setTarget(tTarget.name, tTarget.level, tTarget.healthPercent);

    mIsInRange = tDistance <= kAttackReach;

    if (mIsInRange)
    {
        if (tDistance > 0.001f)
        {
            mEntities.setLocalFacing(std::atan2(tOffset.x, tOffset.y));
        }
    }
    else if (!mAutoWalk.isActive())
    {
        mAutoWalk.follow(mTargetId);
    }
}

bool TargetController::hasTarget() const
{
    return mTargetId != 0;
}

bool TargetController::isInRange() const
{
    return mIsInRange;
}
