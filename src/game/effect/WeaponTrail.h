#ifndef LAKOT_WEAPON_TRAIL_H
#define LAKOT_WEAPON_TRAIL_H

#include <cstdint>
#include <deque>
#include <functional>
#include <unordered_map>

#include <glm/glm.hpp>

namespace lakot
{

struct TrailSample
{
    glm::vec3 tip;
    glm::vec3 base;
    float age;
};

struct WeaponTrackState
{
    std::deque<TrailSample> samples;
    bool isActive{false};
};

class WeaponTrail
{
public:
    using Tracks = std::unordered_map<uint64_t, WeaponTrackState>;

    void setActive(uint64_t pEntityId, bool pIsActive);

    void addSample(uint64_t pEntityId, const glm::vec3& pTip, const glm::vec3& pBase);

    void update(float pDeltaSeconds);

    void retain(const std::function<bool(uint64_t)>& pIsKept);

    void clear();

    const Tracks& getTracks() const;

    static constexpr float kLifetimeSeconds = 0.25f;

private:
    Tracks mTracks;
};

}

#endif
