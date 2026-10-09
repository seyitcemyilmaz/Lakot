#include "AnimationClip.h"

#include <algorithm>
#include <cmath>

using namespace lakot;

namespace
{
    glm::vec3 interpolate(const glm::vec3& pFrom, const glm::vec3& pTo, float pWeight)
    {
        return glm::mix(pFrom, pTo, pWeight);
    }

    glm::quat interpolate(const glm::quat& pFrom, const glm::quat& pTo, float pWeight)
    {
        return glm::slerp(pFrom, pTo, pWeight);
    }

    template <typename T>
    T sampleKeys(const std::vector<AnimationClip::Key<T>>& pKeys, float pTime)
    {
        if (pKeys.size() == 1 || pTime <= pKeys.front().time)
        {
            return pKeys.front().value;
        }

        if (pTime >= pKeys.back().time)
        {
            return pKeys.back().value;
        }

        auto tNext = std::upper_bound(pKeys.begin(), pKeys.end(), pTime,
                                      [](float pValue, const AnimationClip::Key<T>& pKey) { return pValue < pKey.time; });
        auto tPrevious = tNext - 1;

        float tSpan = tNext->time - tPrevious->time;
        float tWeight = tSpan > 0.0f ? (pTime - tPrevious->time) / tSpan : 0.0f;

        return interpolate(tPrevious->value, tNext->value, tWeight);
    }
}

AnimationClip::AnimationClip(std::string pName, float pDuration, std::vector<Track> pTracks)
    : mName(std::move(pName))
    , mDuration(pDuration)
    , mTracks(std::move(pTracks))
{

}

const std::string& AnimationClip::getName() const
{
    return mName;
}

float AnimationClip::getDuration() const
{
    return mDuration;
}

void AnimationClip::setRootMotion(std::vector<Key<glm::vec3>> pKeys)
{
    mRootMotion = std::move(pKeys);
}

glm::vec3 AnimationClip::sampleRootMotion(float pTime) const
{
    return mRootMotion.empty() ? glm::vec3(0.0f) : sampleKeys(mRootMotion, pTime);
}

void AnimationClip::sample(float pTime, std::vector<JointPose>& pPoses) const
{
    float tTime = mDuration > 0.0f ? std::fmod(pTime, mDuration) : 0.0f;

    for (const Track& tTrack : mTracks)
    {
        JointPose& tPose = pPoses[static_cast<size_t>(tTrack.joint)];

        if (!tTrack.translations.empty())
        {
            tPose.translation = sampleKeys(tTrack.translations, tTime);
        }

        if (!tTrack.rotations.empty())
        {
            tPose.rotation = sampleKeys(tTrack.rotations, tTime);
        }

        if (!tTrack.scales.empty())
        {
            tPose.scale = sampleKeys(tTrack.scales, tTime);
        }
    }
}
