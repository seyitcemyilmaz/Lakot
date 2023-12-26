#include "AnimationResource.h"

AnimationResource::AnimationResource(const std::string& pName, double pDuration, double pTicksPerSeconds)
    : mName(pName)
    , mDuration(pDuration)
    , mTicksPerSeconds(pTicksPerSeconds)
{

}

const std::string& AnimationResource::getName() const
{
    return mName;
}

double AnimationResource::getDuration() const
{
    return mDuration;
}

double AnimationResource::getTicksPerSeconds() const
{
    return mTicksPerSeconds;
}

const std::vector<KeyFrameChannelResource*>& AnimationResource::getKeyFrameChannelResources() const
{
    return mKeyFrameChannelResources;
}

unsigned int AnimationResource::getKeyFrameChannelCount() const
{
    return static_cast<int>(mKeyFrameChannelResources.size());
}

void AnimationResource::addKeyFrameChannelResource(KeyFrameChannelResource* pKeyFrameChannelResource)
{
    mKeyFrameChannelResources.push_back(pKeyFrameChannelResource);
}
