#ifndef LAKOT_ANIMATOR_H
#define LAKOT_ANIMATOR_H

#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace lakot
{

class AnimationClip;
class MotionLibrary;
class SkinnedModel;

class Animator
{
public:
    Animator(const SkinnedModel& pModel, MotionLibrary* pLibrary);

    // Cross-fades from whatever is playing; replaying the current clip does nothing.
    void play(const std::string& pClipName, float pFadeSeconds = 0.2f, float pSpeed = 1.0f, float pStart = 0.0f, float pEnd = 0.0f);

    void playOnce(const std::string& pClipName, float pFadeSeconds = 0.1f, bool pHoldLastFrame = false);
    void playSegment(const std::string& pClipName, float pStart, float pEnd, float pSeconds, float pFadeSeconds = 0.1f);
    bool isBusy() const;

    void setGrip(const std::string& pClipName, float pTime);

    void update(float pDeltaTime);

    const std::vector<glm::mat4>& getPalette() const;

    glm::vec3 takeRootMotion();

    // Where the joint currently is, in model space.
    const glm::mat4& getJointTransform(int pJoint) const;

    const std::string& getCurrentClipName() const;

private:
    const AnimationClip* findClip(const std::string& pName) const;
    void start(const AnimationClip* pClip, float pStart, float pLength, float pSpeed, bool pIsOneShot, bool pHoldLastFrame, float pFadeSeconds, bool pIsSegment = false);

    const SkinnedModel& mModel;
    MotionLibrary* mLibrary;

    const AnimationClip* mCurrent{nullptr};
    const AnimationClip* mPrevious{nullptr};
    float mCurrentTime{0.0f};
    bool mIsOneShot{false};
    bool mHoldLastFrame{false};
    float mCurrentStart{0.0f};
    float mCurrentLength{0.0f};
    float mCurrentSpeed{1.0f};
    bool mIsSegment{false};
    glm::vec3 mRootMotion{0.0f};
    float mPreviousTime{0.0f};
    bool mIsPreviousOneShot{false};
    float mPreviousStart{0.0f};
    float mPreviousLength{0.0f};
    float mPreviousSpeed{1.0f};

    float mFadeElapsed{0.0f};
    float mFadeDuration{0.0f};

    std::string mGripClipName;
    std::vector<std::pair<int, glm::quat>> mGrip;

    std::vector<glm::mat4> mPalette;
    std::vector<glm::mat4> mGlobals;
};

}

#endif
