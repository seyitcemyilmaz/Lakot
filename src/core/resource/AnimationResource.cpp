#include "AnimationResource.h"

AnimationResource::AnimationResource(const std::string& pName, double pDuration, double pTicksPerSeconds)
	: mName(pName)
	, mDuration(pDuration)
	, mTicksPerSeconds(pTicksPerSeconds) { }

void AnimationResource::addKeyFrameChannel(KeyFrameChannel* pKeyFrameChannel) {
	mKeyFrameChannels.push_back(pKeyFrameChannel);
}

const std::vector<KeyFrameChannel*>& AnimationResource::getKeyFrameChannels() {
	return mKeyFrameChannels;
}
