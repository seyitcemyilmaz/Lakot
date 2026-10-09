#ifndef LAKOT_AUTO_WALK_H
#define LAKOT_AUTO_WALK_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include <glm/glm.hpp>

#include "MapData.h"

namespace lakot
{

class WorldEntities;

class AutoWalk
{
public:
    explicit AutoWalk(const WorldEntities& pEntities);

    void walkTo(const MapData& pMap, const glm::vec2& pFrom, const glm::vec2& pGoal);
    void follow(uint64_t pPlayerId);
    void stop();

    bool isActive() const;

    glm::vec2 advance(const MapData& pMap, const glm::vec2& pPosition, float pDistance, double pDeltaTime);

    void onRepositioned(const MapData& pMap, const glm::vec2& pPosition);

    std::optional<glm::vec2> getDestination() const;

private:
    const WorldEntities& mEntities;

    std::vector<glm::vec2> mPath;
    size_t mPathIndex{0};
    glm::vec2 mGoal{0.0f};

    bool mIsFollowing{false};
    uint64_t mFollowTargetId{0};
    glm::vec2 mFollowPathGoal{0.0f};
    double mRepathTimer{0.0};
};

}

#endif
