#include "AnimationResource.h"

AnimationResource::AnimationResource(const std::string& pName, double pDuration, double pTicksPerSeconds)
	: mName(pName)
	, mDuration(pDuration)
	, mTicksPerSeconds(pTicksPerSeconds) { }

const std::string& AnimationResource::getName() {
	return mName;
}

double AnimationResource::getDuration() {
	return mDuration;
}

double AnimationResource::getTicksPerSeconds() {
	return mTicksPerSeconds;
}

const std::vector<KeyFrameChannelResource*>& AnimationResource::getKeyFrameChannelResources() {
	return mKeyFrameChannelResources;
}

unsigned int AnimationResource::getKeyFrameChannelCount() {
	return static_cast<int>(mKeyFrameChannelResources.size());
}

void AnimationResource::addKeyFrameChannelResource(KeyFrameChannelResource* pKeyFrameChannelResource) {
	mKeyFrameChannelResources.push_back(pKeyFrameChannelResource);
}
