#include "Animation.h"

#include <spdlog/spdlog.h>

Animation::Animation(AnimationResource* pAnimationResource)
    : mAnimationResource(pAnimationResource)
    , mAnimationStatus(AnimationStatus::eStop)
    , mAnimationPlayType(AnimationPlayType::eNull)
    , mCurrentAnimationTime(0.0)
{

}

void Animation::playAnimation(AnimationPlayType pAnimationPlayType)
{
    if (pAnimationPlayType == AnimationPlayType::eNull)
    {
        spdlog::error("Wrong animation play type");
        return;
    }

    mAnimationStatus = AnimationStatus::ePlay;
    mAnimationPlayType = pAnimationPlayType;
}

void Animation::pauseAnimation()
{
    mAnimationStatus = AnimationStatus::ePause;
}

void Animation::stopAnimation()
{
    mAnimationStatus = AnimationStatus::eStop;
    mAnimationPlayType = AnimationPlayType::eNull;
    mCurrentAnimationTime = 0.0;
}

AnimationStatus Animation::getAnimationStatus()
{
    return mAnimationStatus;
}

AnimationPlayType Animation::getAnimationPlayType()
{
    return mAnimationPlayType;
}

AnimationResource* Animation::getAnimationResource()
{
    return mAnimationResource;
}

const std::string& Animation::getName()
{
    return mAnimationResource->getName();
}

unsigned int Animation::getKeyFrameChannelsCount() const
{
    return static_cast<unsigned int>(mKeyFrameChannels.size());
}

double Animation::getDuration()
{
    return mAnimationResource->getDuration();
}

double Animation::getTicksPerSecond()
{
    return mAnimationResource->getTicksPerSeconds();
}

void Animation::increaseTime(double pTimeDifference)
{
    if (mAnimationStatus != AnimationStatus::ePlay)
    {
        return;
    }

    if (mAnimationPlayType == AnimationPlayType::eNull)
    {
        spdlog::error("Animation type is null.");
        return;
    }

    mCurrentAnimationTime += pTimeDifference * getTicksPerSecond();

    if (mAnimationPlayType == AnimationPlayType::eOnlyOnce)
    {
        if (mCurrentAnimationTime > getDuration())
        {
            stopAnimation();
        }
    }
    else if (mAnimationPlayType == AnimationPlayType::eContinuously)
    {
        if (mCurrentAnimationTime > getDuration())
        {
            mCurrentAnimationTime = std::fmod(mCurrentAnimationTime, getDuration());
        }
    }
}

void Animation::update(double pTimeDifference)
{
    if (mAnimationStatus != AnimationStatus::ePlay)
    {
        return;
    }

    increaseTime(pTimeDifference);

    unsigned int tKeyFrameChannelCount = getKeyFrameChannelsCount();

    for (unsigned int i = 0; i < tKeyFrameChannelCount; i++)
    {
        KeyFrameChannel* tKeyFrameChannel = mKeyFrameChannels[i];

        Node* tNode = tKeyFrameChannel->getNode();

        if (!tNode)
        {
            spdlog::error("Node is not found.");
            continue;
        }

        const KeyFrameChannelResource* tKeyFrameChannelResource = tKeyFrameChannel->getKeyFrameChannelResource();

        const glm::mat4 tTranslation = tKeyFrameChannelResource->interpolatePosition(mCurrentAnimationTime);
        const glm::mat4 tRotation = tKeyFrameChannelResource->interpolateRotation(mCurrentAnimationTime);
        const glm::mat4 tScale = tKeyFrameChannelResource->interpolateScale(mCurrentAnimationTime);

        glm::mat4 tTransformationMatrix = tTranslation * tRotation * tScale;

        tNode->setTransformationMatrix(tTransformationMatrix);
    }
}

void Animation::addKeyFrameChannel(KeyFrameChannel* pKeyFrameChannel)
{
    mKeyFrameChannels.push_back(pKeyFrameChannel);
}
