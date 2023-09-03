#ifndef LAKOT_ANIMATIONRESOURCE_H
#define LAKOT_ANIMATIONRESOURCE_H

#include "KeyFrameChannel.h"

class AnimationResource {
private:
	std::string mName;

	double mDuration;
	double mTicksPerSeconds;

	std::vector<KeyFrameChannel*> mKeyFrameChannels;

public:
	AnimationResource(const std::string& pName, double pDuration, double pTicksPerSeconds);

	const std::vector<KeyFrameChannel*>& getKeyFrameChannels();

	void addKeyFrameChannel(KeyFrameChannel* pKeyFrameChannel);
};

#endif
