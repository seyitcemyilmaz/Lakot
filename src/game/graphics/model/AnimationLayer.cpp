#include "AnimationLayer.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "AnimationClip.h"

using namespace lakot;

namespace
{
    float toClipTime(float pTime, float pStart, float pLength, float pSpeed, bool pIsOneShot)
    {
        float tElapsed = pTime * pSpeed;
        return pStart + (pIsOneShot ? std::min(tElapsed, pLength * 0.999f) : std::fmod(tElapsed, pLength));
    }
}

AnimationLayer::AnimationLayer(const std::vector<JointPose>& pBindPose)
    : mBindPose(pBindPose)
    , mPose(pBindPose)
    , mScratch(pBindPose)
    , mFrozen(pBindPose)
{
}

bool AnimationLayer::isFading() const
{
    return (mPrevious || mHasFrozen) && mFadeElapsed < mFadeDuration;
}

void AnimationLayer::start(const AnimationClip* pClip, float pStart, float pLength, float pSpeed, bool pIsOneShot, bool pHoldLastFrame, float pFadeSeconds, bool pIsSegment)
{
    mIsSegment = pIsSegment;
    mRootMotion = glm::vec3(0.0f);

    if (isFading())
    {
        std::copy(mPose.begin(), mPose.end(), mFrozen.begin());
        mHasFrozen = true;
        mPrevious = nullptr;
    }
    else
    {
        mHasFrozen = false;
        mPrevious = mCurrent;
        mPreviousTime = mCurrentTime;
        mIsPreviousOneShot = mIsOneShot;
        mPreviousStart = mCurrentStart;
        mPreviousLength = mCurrentLength;
        mPreviousSpeed = mCurrentSpeed;
    }

    mCurrent = pClip;
    mCurrentTime = 0.0f;
    mCurrentStart = pStart;
    mCurrentLength = pLength;
    mCurrentSpeed = pSpeed;
    mIsOneShot = pIsOneShot;
    mHoldLastFrame = pHoldLastFrame;

    mFadeElapsed = 0.0f;
    mFadeDuration = (mPrevious || mHasFrozen) ? pFadeSeconds : 0.0f;
}

void AnimationLayer::clear()
{
    mCurrent = nullptr;
    mPrevious = nullptr;
    mHasFrozen = false;
    mIsOneShot = false;
    mHoldLastFrame = false;
    mIsSegment = false;
    mRootMotion = glm::vec3(0.0f);
    mFadeElapsed = 0.0f;
    mFadeDuration = 0.0f;
    std::copy(mBindPose.begin(), mBindPose.end(), mPose.begin());
}

void AnimationLayer::update(float pDeltaTime)
{
    std::copy(mBindPose.begin(), mBindPose.end(), mPose.begin());

    if (mCurrent)
    {
        float tBefore = toClipTime(mCurrentTime, mCurrentStart, mCurrentLength, mCurrentSpeed, mIsOneShot);
        mCurrentTime += pDeltaTime;
        float tAfter = toClipTime(mCurrentTime, mCurrentStart, mCurrentLength, mCurrentSpeed, mIsOneShot);
        mCurrent->sample(tAfter, mPose);

        if (mIsSegment)
        {
            mRootMotion += mCurrent->sampleRootMotion(tAfter) - mCurrent->sampleRootMotion(tBefore);
        }
    }

    if (!isFading())
    {
        mPrevious = nullptr;
        mHasFrozen = false;
        return;
    }

    mFadeElapsed += pDeltaTime;
    const std::vector<JointPose>* tSource = &mFrozen;

    if (mPrevious)
    {
        mPreviousTime += pDeltaTime;
        std::copy(mBindPose.begin(), mBindPose.end(), mScratch.begin());
        mPrevious->sample(toClipTime(mPreviousTime, mPreviousStart, mPreviousLength, mPreviousSpeed, mIsPreviousOneShot), mScratch);
        tSource = &mScratch;
    }

    float tWeight = std::min(mFadeElapsed / mFadeDuration, 1.0f);

    for (size_t tJoint = 0; tJoint < mPose.size(); ++tJoint)
    {
        mPose[tJoint] = JointPose::blend((*tSource)[tJoint], mPose[tJoint], tWeight);
    }
}

const std::vector<JointPose>& AnimationLayer::getPose() const
{
    return mPose;
}

const AnimationClip* AnimationLayer::getClip() const
{
    return mCurrent;
}

bool AnimationLayer::isOneShot() const
{
    return mIsOneShot;
}

bool AnimationLayer::isBusy() const
{
    return mIsOneShot && (mHoldLastFrame || (mCurrent && mCurrentTime * mCurrentSpeed < mCurrentLength));
}

glm::vec3 AnimationLayer::takeRootMotion()
{
    return std::exchange(mRootMotion, glm::vec3(0.0f));
}
