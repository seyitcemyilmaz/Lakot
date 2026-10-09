#ifndef LAKOT_ANIMATION_CLIP_H
#define LAKOT_ANIMATION_CLIP_H

#include <string>
#include <vector>

#include "Skeleton.h"

namespace lakot
{

class AnimationClip
{
public:
    template <typename T>
    struct Key
    {
        float time;
        T value;
    };

    struct Track
    {
        int joint{-1};
        std::vector<Key<glm::vec3>> translations;
        std::vector<Key<glm::quat>> rotations;
        std::vector<Key<glm::vec3>> scales;
    };

    AnimationClip(std::string pName, float pDuration, std::vector<Track> pTracks);

    const std::string& getName() const;
    float getDuration() const;

    // Overwrites the animated joints of pPoses; loops past the end.
    void sample(float pTime, std::vector<JointPose>& pPoses) const;

    void setRootMotion(std::vector<Key<glm::vec3>> pKeys);
    glm::vec3 sampleRootMotion(float pTime) const;

private:
    std::string mName;
    float mDuration;
    std::vector<Track> mTracks;
    std::vector<Key<glm::vec3>> mRootMotion;
};

}

#endif
