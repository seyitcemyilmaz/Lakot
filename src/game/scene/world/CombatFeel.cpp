#include "CombatFeel.h"

#include <algorithm>
#include <cmath>

using namespace lakot;

namespace
{
    constexpr float kTraumaDecayPerSecond = 2.5f;
    constexpr float kShakeAmplitude = 0.12f;
    constexpr double kShakeFrequency = 55.0;
}

void CombatFeel::setCameraShakeEnabled(bool pIsEnabled)
{
    mIsCameraShakeEnabled = pIsEnabled;

    if (!pIsEnabled)
    {
        mTrauma = 0.0f;
    }
}

void CombatFeel::onHit(uint64_t pEntityId, double pStopSeconds, float pTrauma)
{
    double& tRemaining = mStops[pEntityId];
    tRemaining = std::max(tRemaining, pStopSeconds);

    if (mIsCameraShakeEnabled)
    {
        mTrauma = std::min(1.0f, mTrauma + pTrauma);
    }
}

void CombatFeel::update(double pDeltaTime)
{
    mTime += pDeltaTime;
    mTrauma = std::max(0.0f, mTrauma - kTraumaDecayPerSecond * static_cast<float>(pDeltaTime));

    std::erase_if(mStops, [pDeltaTime](auto& pEntry)
    {
        pEntry.second -= pDeltaTime;
        return pEntry.second <= 0.0;
    });
}

float CombatFeel::getAnimationScale(uint64_t pEntityId) const
{
    return mStops.contains(pEntityId) ? 0.0f : 1.0f;
}

glm::vec3 CombatFeel::getCameraOffset(const glm::vec3& pRight) const
{
    if (mTrauma <= 0.0f)
    {
        return glm::vec3(0.0f);
    }

    float tSide = static_cast<float>(std::sin(mTime * kShakeFrequency));
    float tUp = static_cast<float>(std::sin(mTime * kShakeFrequency * 1.37 + 1.7));

    return (pRight * tSide + glm::vec3(0.0f, tUp, 0.0f)) * (mTrauma * kShakeAmplitude);
}

void CombatFeel::clear()
{
    mStops.clear();
    mTrauma = 0.0f;
}
