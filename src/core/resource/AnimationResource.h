#ifndef LAKOT_ANIMATIONRESOURCE_H
#define LAKOT_ANIMATIONRESOURCE_H

#include "KeyFrameChannelResource.h"

class AnimationResource {
private:
	std::string mName;

	double mDuration;
	double mTicksPerSeconds;
	
	std::vector<KeyFrameChannelResource*> mKeyFrameChannelResources;

public:
	AnimationResource(const std::string& pName, double pDuration, double pTicksPerSeconds);

	const std::string& getName();

	double getDuration();
	double getTicksPerSeconds();
	
	const std::vector<KeyFrameChannelResource*>& getKeyFrameChannelResources();
	unsigned int getKeyFrameChannelCount();
	
	void addKeyFrameChannelResource(KeyFrameChannelResource* pKeyFrameChannelResource);
};

#endif
