#ifndef LAKOT_ANIMATION_LAYER_H
#define LAKOT_ANIMATION_LAYER_H

#include <vector>

#include <glm/glm.hpp>

#include "Skeleton.h"

namespace lakot
{

class AnimationClip;

class AnimationLayer
{
public:
    explicit AnimationLayer(const std::vector<JointPose>& pBindPose);

    void start(const AnimationClip* pClip, float pStart, float pLength, float pSpeed, bool pIsOneShot, bool pHoldLastFrame, float pFadeSeconds, bool pIsSegment);
    void clear();
    void update(float pDeltaTime);

    const std::vector<JointPose>& getPose() const;
    const AnimationClip* getClip() const;
    bool isOneShot() const;
    bool isBusy() const;
    glm::vec3 takeRootMotion();

private:
    bool isFading() const;

    const std::vector<JointPose>& mBindPose;

    std::vector<JointPose> mPose;
    std::vector<JointPose> mScratch;
    std::vector<JointPose> mFrozen;
    bool mHasFrozen{false};

    const AnimationClip* mCurrent{nullptr};
    const AnimationClip* mPrevious{nullptr};
    float mCurrentTime{0.0f};
    bool mIsOneShot{false};
    bool mHoldLastFrame{false};
    float mCurrentStart{0.0f};
    float mCurrentLength{0.0f};
    float mCurrentSpeed{1.0f};
    bool mIsSegment{false};
    glm::vec3 mRootMotion{0.0f};
    float mPreviousTime{0.0f};
    bool mIsPreviousOneShot{false};
    float mPreviousStart{0.0f};
    float mPreviousLength{0.0f};
    float mPreviousSpeed{1.0f};

    float mFadeElapsed{0.0f};
    float mFadeDuration{0.0f};
};

}

#endif
