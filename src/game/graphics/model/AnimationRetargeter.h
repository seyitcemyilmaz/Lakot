#ifndef LAKOT_ANIMATION_RETARGETER_H
#define LAKOT_ANIMATION_RETARGETER_H

#include <memory>

#include "AnimationClip.h"
#include "Skeleton.h"

namespace lakot
{

class AnimationRetargeter
{
public:
    static std::unique_ptr<AnimationClip> retarget(const AnimationClip& pClip, const Skeleton& pSource, const Skeleton& pTarget);

private:
    static constexpr float kSampleRate = 30.0f;

    AnimationRetargeter() = delete;
    ~AnimationRetargeter() = delete;
};

}

#endif
