#include "Animator.h"

#include <algorithm>
#include <cmath>

#include <SDL3/SDL_log.h>

#include "AnimationClip.h"
#include "MotionLibrary.h"
#include "SkinnedModel.h"

using namespace lakot;

Animator::Animator(const SkinnedModel& pModel, MotionLibrary* pLibrary)
    : mModel(pModel)
    , mLibrary(pLibrary)
    , mBindPose(pModel.getSkeleton().getBindPose())
    , mBase(mBindPose)
    , mPose(mBindPose)
{
    mModel.getSkeleton().computePalette(mBindPose, mPalette, mGlobals);
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
    const AnimationClip* tClip = mBase.getClip();
    return tClip ? tClip->getName() : sNone;
}

void Animator::play(const std::string& pClipName, float pFadeSeconds, float pSpeed, float pStart, float pEnd)
{
    const AnimationClip* tClip = findClip(pClipName);

    if (!tClip || (tClip == mBase.getClip() && !mBase.isOneShot()))
    {
        return;
    }

    float tStart = std::clamp(pStart, 0.0f, tClip->getDuration());
    float tEnd = pEnd > tStart ? std::min(pEnd, tClip->getDuration()) : tClip->getDuration();
    mBase.start(tClip, tStart, std::max(tEnd - tStart, 0.001f), pSpeed, false, false, pFadeSeconds, false);
}

void Animator::playOnce(const std::string& pClipName, float pFadeSeconds, bool pHoldLastFrame)
{
    const AnimationClip* tClip = findClip(pClipName);

    if (!tClip)
    {
        return;
    }

    mBase.start(tClip, 0.0f, tClip->getDuration(), 1.0f, true, pHoldLastFrame, pFadeSeconds, false);
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
    mBase.start(tClip, tStart, tLength, pSeconds > 0.0f ? tLength / pSeconds : 1.0f, true, false, pFadeSeconds, true);
}

bool Animator::isBusy() const
{
    return mBase.isBusy();
}

void Animator::ensureUpperLayer()
{
    if (mUpper)
    {
        return;
    }

    mUpper = std::make_unique<AnimationLayer>(mBindPose);
    const Skeleton& tSkeleton = mModel.getSkeleton();
    int tRoot = tSkeleton.findJointByBaseName("Spine1");

    if (tRoot < 0)
    {
        tRoot = tSkeleton.findJointByBaseName("Spine");
    }

    if (tRoot < 0)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Animator: no Spine1/Spine joint, the upper body layer covers the whole body");
        mUpperMask.assign(tSkeleton.getJointCount(), 1.0f);
        return;
    }

    tSkeleton.collectDescendants(tRoot, mUpperMask);
}

void Animator::playUpper(const std::string& pClipName, float pFadeSeconds, float pSpeed, float pStart, float pEnd, bool pIsOneShot)
{
    const AnimationClip* tClip = findClip(pClipName);

    if (!tClip)
    {
        return;
    }

    ensureUpperLayer();

    if (mUpperTarget > 0.0f && tClip == mUpper->getClip() && !pIsOneShot && !mUpper->isOneShot())
    {
        return;
    }

    float tStart = std::clamp(pStart, 0.0f, tClip->getDuration());
    float tEnd = pEnd > tStart ? std::min(pEnd, tClip->getDuration()) : tClip->getDuration();
    float tLength = std::max(tEnd - tStart, 0.001f);

    if (mUpperWeight <= 0.0f)
    {
        mUpper->clear();
    }

    mUpper->start(tClip, tStart, tLength, pSpeed, pIsOneShot, false, pFadeSeconds, false);
    mUpperTarget = 1.0f;
    mUpperFadeSeconds = pFadeSeconds;

    if (pFadeSeconds <= 0.0f)
    {
        mUpperWeight = 1.0f;
    }
}

void Animator::stopUpper(float pFadeSeconds)
{
    mUpperTarget = 0.0f;
    mUpperFadeSeconds = pFadeSeconds;

    if (pFadeSeconds <= 0.0f)
    {
        mUpperWeight = 0.0f;
    }
}

bool Animator::isUpperBusy() const
{
    return mUpper && mUpperTarget > 0.0f && mUpper->isBusy();
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

    std::vector<JointPose> tPose = mBindPose;
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
    mBase.update(pDeltaTime);
    std::copy(mBase.getPose().begin(), mBase.getPose().end(), mPose.begin());

    if (mUpper)
    {
        float tStep = mUpperFadeSeconds > 0.0f ? pDeltaTime / mUpperFadeSeconds : 1.0f;
        mUpperWeight = mUpperTarget > mUpperWeight ? std::min(mUpperWeight + tStep, mUpperTarget) : std::max(mUpperWeight - tStep, mUpperTarget);

        if (mUpperWeight > 0.0f)
        {
            mUpper->update(pDeltaTime);
            const std::vector<JointPose>& tUpperPose = mUpper->getPose();

            for (size_t tJoint = 0; tJoint < mPose.size(); ++tJoint)
            {
                float tWeight = mUpperWeight * mUpperMask[tJoint];

                if (tWeight > 0.0f)
                {
                    mPose[tJoint] = JointPose::blend(mPose[tJoint], tUpperPose[tJoint], tWeight);
                }
            }
        }
        else if (mUpperTarget <= 0.0f)
        {
            mUpper->clear();
        }
    }

    for (const auto& [tJoint, tRotation] : mGrip)
    {
        mPose[static_cast<size_t>(tJoint)].rotation = tRotation;
    }

    mModel.getSkeleton().computePalette(mPose, mPalette, mGlobals);
}

glm::vec3 Animator::takeRootMotion()
{
    return mBase.takeRootMotion();
}

const std::vector<glm::mat4>& Animator::getPalette() const
{
    return mPalette;
}

const glm::mat4& Animator::getJointTransform(int pJoint) const
{
    static const glm::mat4 sIdentity(1.0f);

    if (pJoint < 0 || static_cast<size_t>(pJoint) >= mGlobals.size())
    {
        return sIdentity;
    }

    return mGlobals[static_cast<size_t>(pJoint)];
}
