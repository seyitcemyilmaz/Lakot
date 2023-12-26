#ifndef LAKOT_IANIMATED_H
#define LAKOT_IANIMATED_H

#include <glm/glm.hpp>

#include "INode.h"

class IAnimated
{
public:
    IAnimated();

    virtual void playAnimationContinuously(unsigned int pIndex) = 0;
    virtual void playAnimationOnlyOnce(unsigned int pIndex) = 0;
    virtual void pauseAnimation(unsigned int pIndex) = 0;
    virtual void stopAnimation(unsigned int pIndex) = 0;

protected:
    virtual void calculateBoneTransformations(INode* pNode, const glm::mat4& tParentTransform) = 0;
    virtual void updateAnimations(double pTimeDifference) = 0;
};

#endif
