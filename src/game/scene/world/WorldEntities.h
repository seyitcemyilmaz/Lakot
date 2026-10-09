#ifndef LAKOT_WORLD_ENTITIES_H
#define LAKOT_WORLD_ENTITIES_H

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "WorldEntity.h"

#include "../../network/controller/WorldController.h"

namespace lakot
{

class AssetManager;
class ItemAppearance;
class MonsterCatalog;
class MotionLibrary;

struct DamageEvent
{
    uint64_t targetId;
    uint32_t damage;
    bool isAgainstLocal;
    bool isByLocal;
};

class WorldEntities
{
public:
    static constexpr uint64_t kLocalId = std::numeric_limits<uint64_t>::max();

    WorldEntities(AssetManager& pAssets, const MonsterCatalog& pMonsterCatalog, MotionLibrary& pMotions, ItemAppearance& pAppearance);

    void setLocalCharacterId(uint64_t pCharacterId);
    const WorldEntity* getLocal() const;

    void playLocalAttack(uint32_t pCombo);
    std::vector<DamageEvent> takeDamageEvents();

    glm::vec2 takeLocalRootMotion();

    void applySnapshot(const WorldController::WorldSnapshot& pSnapshot);

    // Everyone but the local player.
    void clearRemote();

    void setLocal(const std::string& pName, const glm::vec3& pPosition, double pStepSeconds);
    void teleportLocal(const glm::vec3& pPosition);

    void setLocalEquipment(const std::vector<OwnedItem>& pItems);

    void showSpeech(uint64_t pId, const std::string& pText);

    void update(double pDeltaTime);

    const std::unordered_map<uint64_t, WorldEntity>& getAll() const;
    std::optional<glm::vec3> findPosition(uint64_t pId) const;

    // pExtraSeconds: how far past the last fixed update the frame is drawn.
    static glm::vec3 getPosition(const WorldEntity& pEntity, double pExtraSeconds = 0.0);

private:
    static constexpr const char* kPlayerBodyModel = "models/warrior/body.glb";
    static constexpr const char* kMonsterPlaceholderModel = "models/monsters/placeholder.glb";
    static constexpr const char* kMonsterFallbackModel = "models/monsters/steppe_bandit.glb";

    AssetManager& mAssets;
    const MonsterCatalog& mMonsterCatalog;
    MotionLibrary& mMotions;
    ItemAppearance& mAppearance;
    std::unordered_map<uint64_t, WorldEntity> mEntities;
    uint64_t mLocalCharacterId{0};
    std::vector<DamageEvent> mDamageEvents;
    std::unordered_set<std::string> mFallbackLogged;

    uint64_t toLocalId(uint64_t pEntityId) const;
    WorldEntity* find(uint64_t pEntityId);

    std::shared_ptr<SkinnedModel> loadMonsterModel(const std::string& pModelPath);
    WorldEntity& spawn(uint64_t pId, EntityType pType, const std::string& pName, const glm::vec3& pPosition,
                       const std::string& pModelPath = {});
    void animate(WorldEntity& pEntity, double pDeltaTime);
    void setModel(WorldEntity& pEntity, std::shared_ptr<SkinnedModel> pModel);
    void refreshBody(WorldEntity& pEntity);
    static void playRole(WorldEntity& pEntity, AnimationRoleType pRole, bool pIsOnce, bool pHold = false);
    static void applyDeath(WorldEntity& pEntity, bool pIsDead);
};

}

#endif
