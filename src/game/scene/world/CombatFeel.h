#ifndef LAKOT_COMBAT_FEEL_H
#define LAKOT_COMBAT_FEEL_H

#include <cstdint>
#include <unordered_map>

#include <glm/glm.hpp>

namespace lakot
{

class CombatFeel
{
public:
    void setCameraShakeEnabled(bool pIsEnabled);

    void onHit(uint64_t pEntityId, double pStopSeconds, float pTrauma);

    void update(double pDeltaTime);

    float getAnimationScale(uint64_t pEntityId) const;

    glm::vec3 getCameraOffset(const glm::vec3& pRight) const;

    void clear();

private:
    std::unordered_map<uint64_t, double> mStops;
    bool mIsCameraShakeEnabled{true};
    float mTrauma{0.0f};
    double mTime{0.0};
};

}

#endif
