#ifndef LAKOT_ANIMATION_H
#define LAKOT_ANIMATION_H

#include "resource/AnimationResource.h"

enum class AnimationStatus {
	eStop,
	ePlay,
	ePause
};

class Animation {
private:
	AnimationResource* mAnimationResource;

	AnimationStatus mAnimationStatus = AnimationStatus::eStop;

	double mCurrentAnimationTime = 0.0;

public:
	Animation(AnimationResource* pAnimationResource);

	AnimationResource* getAnimationResource();

	const std::string& getName();

	double getDuration();
	double getTicksPerSecond();

	void playAnimation();
	void pauseAnimation();
	void stopAnimation();

	void increaseTime(double pTimeDifference);
};

#endif // ANIMATION_H
