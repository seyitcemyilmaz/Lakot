#ifndef LAKOT_ANIMATOR_H
#define LAKOT_ANIMATOR_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "AnimationLayer.h"
#include "Skeleton.h"

namespace lakot
{

class AnimationClip;
class MotionLibrary;
class SkinnedModel;

class Animator
{
public:
    Animator(const SkinnedModel& pModel, MotionLibrary* pLibrary);
    Animator(const Animator&) = delete;
    Animator& operator=(const Animator&) = delete;

    // Cross-fades from whatever is playing; replaying the current clip does nothing.
    void play(const std::string& pClipName, float pFadeSeconds = 0.2f, float pSpeed = 1.0f, float pStart = 0.0f, float pEnd = 0.0f);

    void playOnce(const std::string& pClipName, float pFadeSeconds = 0.1f, bool pHoldLastFrame = false);
    void playSegment(const std::string& pClipName, float pStart, float pEnd, float pSeconds, float pFadeSeconds = 0.1f);
    bool isBusy() const;

    void playUpper(const std::string& pClipName, float pFadeSeconds = 0.2f, float pSpeed = 1.0f, float pStart = 0.0f, float pEnd = 0.0f, bool pIsOneShot = false);
    void stopUpper(float pFadeSeconds = 0.2f);
    bool isUpperBusy() const;

    void setGrip(const std::string& pClipName, float pTime);

    void update(float pDeltaTime);

    const std::vector<glm::mat4>& getPalette() const;

    glm::vec3 takeRootMotion();
    void takeEvents(std::vector<ClipEvent>& pOut);

    // Where the joint currently is, in model space.
    const glm::mat4& getJointTransform(int pJoint) const;

    const std::string& getCurrentClipName() const;

private:
    const AnimationClip* findClip(const std::string& pName) const;
    const ClipDefinition* findDefinition(const std::string& pName) const;
    void ensureUpperLayer();

    const SkinnedModel& mModel;
    MotionLibrary* mLibrary;

    std::vector<JointPose> mBindPose;
    AnimationLayer mBase;
    std::unique_ptr<AnimationLayer> mUpper;
    std::vector<float> mUpperMask;
    float mUpperWeight{0.0f};
    float mUpperTarget{0.0f};
    float mUpperFadeSeconds{0.0f};
    std::vector<JointPose> mPose;

    std::string mGripClipName;
    std::vector<std::pair<int, glm::quat>> mGrip;

    std::vector<glm::mat4> mPalette;
    std::vector<glm::mat4> mGlobals;
};

}

#endif
