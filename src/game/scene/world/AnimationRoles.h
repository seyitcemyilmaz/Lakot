#ifndef LAKOT_ANIMATION_ROLES_H
#define LAKOT_ANIMATION_ROLES_H

#include <array>
#include <cstddef>
#include <string>

namespace lakot
{

class MotionLibrary;
class SkinnedModel;

enum class AnimationRoleType
{
    eIdle,
    eWalk,
    eRun,
    eAttack1,
    eAttack2,
    eAttack3,
    eHurt,
    eDeath,
    eGrip,
    eCount
};

struct AnimationRoleClip
{
    std::string name;
    float start = 0.0f;
    float end = 0.0f;
    float rate = 1.0f;

    bool isSegment() const { return end > start; }
};

using AnimationRoleClips = std::array<AnimationRoleClip, static_cast<size_t>(AnimationRoleType::eCount)>;

class AnimationRoles
{
public:
    static AnimationRoleClips resolve(const SkinnedModel& pModel, const MotionLibrary* pLibrary);

private:
    AnimationRoles() = delete;
    ~AnimationRoles() = delete;
};

}

#endif
