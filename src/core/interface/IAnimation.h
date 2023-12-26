#ifndef LAKOT_IANIMATION_H
#define LAKOT_IANIMATION_H

enum class AnimationStatus
{
    eStop,
    ePlay,
    ePause
};

enum class AnimationPlayType
{
    eNull,
    eOnlyOnce,
    eContinuously
};

class IAnimation
{
public:
    IAnimation();

    virtual void playAnimation(AnimationPlayType pAnimationPlayType) = 0;
    virtual void pauseAnimation() = 0;
    virtual void stopAnimation() = 0;

    virtual AnimationStatus getAnimationStatus() = 0;
    virtual AnimationPlayType getAnimationPlayType() = 0;
};

#endif
