#include "Animation.h"

Animation::Animation(AnimationResource* pAnimationResource)
	: mAnimationResource(pAnimationResource) { }

AnimationResource* Animation::getAnimationResource() {
	return mAnimationResource;
}

const std::string& Animation::getName() {
	return mAnimationResource->getName();
}

double Animation::getDuration() {
	return mAnimationResource->getDuration();
}

double Animation::getTicksPerSecond() {
	return mAnimationResource->getTicksPerSeconds();
}

void Animation::playAnimation() {
	mAnimationStatus = AnimationStatus::ePlay;
}

void Animation::pauseAnimation() {
	mAnimationStatus = AnimationStatus::ePause;
}

void Animation::stopAnimation() {
	mAnimationStatus = AnimationStatus::eStop;
}

void Animation::increaseTime(double pTimeDifference) {
	if (mAnimationStatus == AnimationStatus::eStop) {
		return;
	}

	if (mAnimationStatus == AnimationStatus::ePause) {
		return;
	}

	if (mAnimationStatus == AnimationStatus::ePlay) {
		mCurrentAnimationTime += pTimeDifference * getTicksPerSecond();

		if (mCurrentAnimationTime > getDuration()) {
			stopAnimation();
		}

		return;
	}

	throw "Unknown animation status";
}
