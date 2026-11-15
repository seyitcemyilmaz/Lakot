#ifndef LAKOT_TARGET_CONTROLLER_H
#define LAKOT_TARGET_CONTROLLER_H

#include <cstdint>

#include <glm/glm.hpp>

namespace lakot
{

class AutoWalk;
class WorldEntities;
class WorldHud;

class TargetController
{
public:
    TargetController(WorldEntities& pEntities, AutoWalk& pAutoWalk, WorldHud& pHud);

    void select(uint64_t pEntityId);
    void clear();
    void update(const glm::vec3& pLocalPosition, bool pIsLocalDead);

    bool hasTarget() const;
    bool isInRange() const;

private:
    WorldEntities& mEntities;
    AutoWalk& mAutoWalk;
    WorldHud& mHud;

    uint64_t mTargetId{0};
    bool mIsInRange{false};
};

}

#endif
