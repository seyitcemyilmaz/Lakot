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

void AnimationResource::addKeyFrameChannel(KeyFrameChannel* pKeyFrameChannel) {
	mKeyFrameChannels.push_back(pKeyFrameChannel);
}

const std::vector<KeyFrameChannel*>& AnimationResource::getKeyFrameChannels() {
	return mKeyFrameChannels;
}
