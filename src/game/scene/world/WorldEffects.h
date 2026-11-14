#ifndef LAKOT_WORLD_EFFECTS_H
#define LAKOT_WORLD_EFFECTS_H

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "../../effect/EffectSystem.h"
#include "../../effect/ParticleRenderer.h"
#include "../../network/controller/WorldController.h"

namespace lakot
{

class AssetManager;
class Camera;
class ShaderManager;
class WorldEntities;

class WorldEffects
{
public:
    WorldEffects(AssetManager& pAssets, ShaderManager& pShaders, const WorldEntities& pEntities, uint32_t pInitialLevel);

    void onSnapshot(const WorldController::WorldSnapshot& pSnapshot);
    void update(double pDeltaSeconds);
    void render(const Camera& pCamera, const glm::vec3& pFogColor, float pFogStart, float pFogEnd, double pExtraSeconds);
    void clear();

    bool reload(const std::string& pPath, std::string& pError);

    EffectSystem& getSystem();
    const ParticleRenderer& getRenderer() const;

private:
    static constexpr const char* kHitSpark = "hit_spark";
    static constexpr const char* kHitKill = "hit_kill";
    static constexpr const char* kLevelUp = "level_up";
    static constexpr const char* kLootDrop = "loot_drop";
    static constexpr const char* kLootDropGold = "loot_drop_gold";

    static constexpr double kKillMemorySeconds = 2.0;
    static constexpr float kKillLootDistance = 2.0f;
    static constexpr float kSelfLootDistance = 3.0f;
    static constexpr double kLootGraceSeconds = 2.0;

    struct KillRecord
    {
        glm::vec3 position;
        double time;
    };

    const WorldEntities& mEntities;
    EffectSystem mSystem;
    ParticleRenderer mRenderer;

    std::vector<KillRecord> mKills;
    double mTime{0.0};
    double mLootGraceRemaining{kLootGraceSeconds};
    uint32_t mLastLevel;

    void onHits(const WorldController::CombatSnapshot& pCombat);
    void onLoot(const WorldController::EntitySnapshot& pEntity);
    bool isNearRecentKill(const glm::vec3& pPosition) const;
    bool isNearLocal(const glm::vec3& pPosition, float pDistance) const;
    void spawnNearLocal(const char* pName, const glm::vec3& pPosition);
};

}

#endif
