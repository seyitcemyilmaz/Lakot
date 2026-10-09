#include "AnimationRoles.h"

#include <initializer_list>

#include "../../graphics/model/MotionLibrary.h"
#include "../../graphics/model/SkinnedModel.h"

using namespace lakot;

namespace
{
    const std::array<std::initializer_list<AnimationRoleClip>, static_cast<size_t>(AnimationRoleType::eCount)> kCandidates = {{
        { { "Idle" } },
        { },
        { { "Run" } },
        { },
        { },
        { },
        { },
        { },
        { { "Run" } },
    }};
}

AnimationRoleClips AnimationRoles::resolve(const SkinnedModel& pModel, const MotionLibrary* pLibrary)
{
    AnimationRoleClips tClips;

    for (size_t tRole = 0; tRole < kCandidates.size(); ++tRole)
    {
        for (const AnimationRoleClip& tCandidate : kCandidates[tRole])
        {
            if ((pLibrary && pLibrary->hasClip(tCandidate.name)) || pModel.findClip(tCandidate.name))
            {
                tClips[tRole] = tCandidate;
                break;
            }
        }
    }

    return tClips;
}
