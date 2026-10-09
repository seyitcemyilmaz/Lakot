#include "CameraBob.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/constants.hpp>

using namespace lakot;

namespace
{
    constexpr float kStepLength = 1.55f;
    constexpr float kDip = 0.1f;
    constexpr float kSway = 0.05f;
    constexpr float kMovingSpeed = 0.5f;
    constexpr float kFadeRate = 6.0f;
}

void CameraBob::update(float pDistance, double pDeltaTime)
{
    float tDeltaTime = static_cast<float>(pDeltaTime);
    bool tIsMoving = tDeltaTime > 0.0f && pDistance / tDeltaTime > kMovingSpeed;

    mWeight += ((tIsMoving ? 1.0f : 0.0f) - mWeight) * std::min(1.0f, kFadeRate * tDeltaTime);
    mPhase = std::fmod(mPhase + pDistance / kStepLength * glm::pi<float>(), glm::two_pi<float>());

    mPrevious = mCurrent;
    mCurrent = glm::vec2(kSway * std::sin(mPhase), -kDip * 0.5f * (1.0f - std::cos(2.0f * mPhase))) * mWeight;
}

glm::vec3 CameraBob::getOffset(const glm::vec3& pRight, float pAlpha) const
{
    glm::vec2 tOffset = glm::mix(mPrevious, mCurrent, pAlpha);
    glm::vec3 tSide = glm::vec3(pRight.x, 0.0f, pRight.z);

    if (glm::length(tSide) > 0.0001f)
    {
        tSide = glm::normalize(tSide);
    }

    return tSide * tOffset.x + glm::vec3(0.0f, tOffset.y, 0.0f);
}
