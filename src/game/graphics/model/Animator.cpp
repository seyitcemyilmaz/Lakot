#include "Animator.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "MotionLibrary.h"
#include "SkinnedModel.h"

using namespace lakot;

namespace
{
    float toClipTime(float pTime, float pStart, float pLength, float pSpeed, bool pIsOneShot)
    {
        float tElapsed = pTime * pSpeed;
        return pStart + (pIsOneShot ? std::min(tElapsed, pLength * 0.999f) : std::fmod(tElapsed, pLength));
    }
}

Animator::Animator(const SkinnedModel& pModel, MotionLibrary* pLibrary)
    : mModel(pModel)
    , mLibrary(pLibrary)
{
    mModel.getSkeleton().computePalette(mModel.getSkeleton().getBindPose(), mPalette, mGlobals);
}

const AnimationClip* Animator::findClip(const std::string& pName) const
{
    if (mLibrary && mLibrary->hasClip(pName))
    {
        return mLibrary->getClip(mModel, pName);
    }

    return mModel.findClip(pName);
}

const std::string& Animator::getCurrentClipName() const
{
    static const std::string sNone;
    return mCurrent ? mCurrent->getName() : sNone;
}

void Animator::play(const std::string& pClipName, float pFadeSeconds, float pSpeed, float pStart, float pEnd)
{
    const AnimationClip* tClip = findClip(pClipName);

    if (!tClip || (tClip == mCurrent && !mIsOneShot))
    {
        return;
    }

    float tStart = std::clamp(pStart, 0.0f, tClip->getDuration());
    float tEnd = pEnd > tStart ? std::min(pEnd, tClip->getDuration()) : tClip->getDuration();
    start(tClip, tStart, std::max(tEnd - tStart, 0.001f), pSpeed, false, false, pFadeSeconds);
}

void Animator::playOnce(const std::string& pClipName, float pFadeSeconds, bool pHoldLastFrame)
{
    const AnimationClip* tClip = findClip(pClipName);

    if (!tClip)
    {
        return;
    }

    start(tClip, 0.0f, tClip->getDuration(), 1.0f, true, pHoldLastFrame, pFadeSeconds);
}

void Animator::playSegment(const std::string& pClipName, float pStart, float pEnd, float pSeconds, float pFadeSeconds)
{
    const AnimationClip* tClip = findClip(pClipName);

    if (!tClip)
    {
        return;
    }

    float tStart = std::clamp(pStart, 0.0f, tClip->getDuration());
    float tLength = std::max(std::min(pEnd, tClip->getDuration()) - tStart, 0.001f);
    start(tClip, tStart, tLength, pSeconds > 0.0f ? tLength / pSeconds : 1.0f, true, false, pFadeSeconds, true);
}

void Animator::start(const AnimationClip* pClip, float pStart, float pLength, float pSpeed, bool pIsOneShot, bool pHoldLastFrame, float pFadeSeconds, bool pIsSegment)
{
    mIsSegment = pIsSegment;
    mRootMotion = glm::vec3(0.0f);

    mPrevious = mCurrent;
    mPreviousTime = mCurrentTime;
    mIsPreviousOneShot = mIsOneShot;
    mPreviousStart = mCurrentStart;
    mPreviousLength = mCurrentLength;
    mPreviousSpeed = mCurrentSpeed;

    mCurrent = pClip;
    mCurrentTime = 0.0f;
    mCurrentStart = pStart;
    mCurrentLength = pLength;
    mCurrentSpeed = pSpeed;
    mIsOneShot = pIsOneShot;
    mHoldLastFrame = pHoldLastFrame;

    mFadeElapsed = 0.0f;
    mFadeDuration = mPrevious ? pFadeSeconds : 0.0f;
}

bool Animator::isBusy() const
{
    return mIsOneShot && (mHoldLastFrame || (mCurrent && mCurrentTime * mCurrentSpeed < mCurrentLength));
}

void Animator::setGrip(const std::string& pClipName, float pTime)
{
    if (pClipName == mGripClipName)
    {
        return;
    }

    mGripClipName = pClipName;
    mGrip.clear();

    const AnimationClip* tClip = pClipName.empty() ? nullptr : findClip(pClipName);
    const Skeleton& tSkeleton = mModel.getSkeleton();
    int tHand = tSkeleton.findJointByBaseName("RightHand");

    if (!tClip || tHand < 0)
    {
        return;
    }

    std::vector<JointPose> tPose = tSkeleton.getBindPose();
    tClip->sample(pTime, tPose);

    const auto& tJoints = tSkeleton.getJoints();
    std::vector<bool> tIsFinger(tJoints.size(), false);

    for (size_t tJoint = 0; tJoint < tJoints.size(); ++tJoint)
    {
        int tParent = tJoints[tJoint].parent;

        if (tParent >= 0 && (tParent == tHand || tIsFinger[static_cast<size_t>(tParent)]))
        {
            tIsFinger[tJoint] = true;
            mGrip.emplace_back(static_cast<int>(tJoint), tPose[tJoint].rotation);
        }
    }
}

void Animator::update(float pDeltaTime)
{
    const Skeleton& tSkeleton = mModel.getSkeleton();
    std::vector<JointPose> tPose = tSkeleton.getBindPose();

    if (mCurrent)
    {
        float tBefore = toClipTime(mCurrentTime, mCurrentStart, mCurrentLength, mCurrentSpeed, mIsOneShot);
        mCurrentTime += pDeltaTime;
        float tAfter = toClipTime(mCurrentTime, mCurrentStart, mCurrentLength, mCurrentSpeed, mIsOneShot);
        mCurrent->sample(tAfter, tPose);

        if (mIsSegment)
        {
            mRootMotion += mCurrent->sampleRootMotion(tAfter) - mCurrent->sampleRootMotion(tBefore);
        }
    }

    if (mPrevious && mFadeElapsed < mFadeDuration)
    {
        mPreviousTime += pDeltaTime;
        mFadeElapsed += pDeltaTime;

        std::vector<JointPose> tPreviousPose = tSkeleton.getBindPose();
        mPrevious->sample(toClipTime(mPreviousTime, mPreviousStart, mPreviousLength, mPreviousSpeed, mIsPreviousOneShot), tPreviousPose);

        float tWeight = std::min(mFadeElapsed / mFadeDuration, 1.0f);

        for (size_t tJoint = 0; tJoint < tPose.size(); ++tJoint)
        {
            tPose[tJoint] = JointPose::blend(tPreviousPose[tJoint], tPose[tJoint], tWeight);
        }
    }
    else
    {
        mPrevious = nullptr;
    }

    for (const auto& [tJoint, tRotation] : mGrip)
    {
        tPose[static_cast<size_t>(tJoint)].rotation = tRotation;
    }

    tSkeleton.computePalette(tPose, mPalette, mGlobals);
}

glm::vec3 Animator::takeRootMotion()
{
    return std::exchange(mRootMotion, glm::vec3(0.0f));
}

const std::vector<glm::mat4>& Animator::getPalette() const
{
    return mPalette;
}

const glm::mat4& Animator::getJointTransform(int pJoint) const
{
    return mGlobals[static_cast<size_t>(pJoint)];
}
