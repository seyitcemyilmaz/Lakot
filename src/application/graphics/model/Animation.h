#ifndef LAKOT_ANIMATION_H
#define LAKOT_ANIMATION_H

#include "resource/AnimationResource.h"

#include "interface/IAnimation.h"

#include "KeyFrameChannel.h"

class Animation : public IAnimation {
private:
	AnimationResource* mAnimationResource;

	AnimationStatus mAnimationStatus;
	AnimationPlayType mAnimationPlayType;

	double mCurrentAnimationTime;

	std::vector<KeyFrameChannel*> mKeyFrameChannels;

public:
	Animation(AnimationResource* pAnimationResource);

	void playAnimation(AnimationPlayType pAnimationPlayType) override;
	void pauseAnimation() override;
	void stopAnimation() override;

	AnimationStatus getAnimationStatus() override;
	AnimationPlayType getAnimationPlayType() override;

	AnimationResource* getAnimationResource();

	const std::string& getName();

	unsigned int getKeyFrameChannelsCount() const;

	double getDuration();
	double getTicksPerSecond();

	void increaseTime(double pTimeDifference);
	void update(double pTimeDifference);

	void addKeyFrameChannel(KeyFrameChannel* pKeyFrameChannel);
};

#endif
