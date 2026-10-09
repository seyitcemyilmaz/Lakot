#include "WorldEntities.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <SDL3/SDL_log.h>
#include <glm/gtc/constants.hpp>

#include "WorldViewConstants.h"

#include "../../asset/AssetManager.h"
#include "MonsterCatalog.h"
#include "../../graphics/model/MotionLibrary.h"
#include "../../item/ItemAppearance.h"

using namespace lakot;

namespace
{
    constexpr float kMovingSpeed = 0.5f;
    constexpr double kMovingHoldSeconds = 0.2;
    constexpr float kTurnSpeed = 12.0f;

    float turnTowards(float pFrom, float pTo, float pMaxStep)
    {
        float tDifference = std::remainder(pTo - pFrom, glm::two_pi<float>());
        return pFrom + std::clamp(tDifference, -pMaxStep, pMaxStep);
    }
}

WorldEntities::WorldEntities(AssetManager& pAssets, const MonsterCatalog& pMonsterCatalog, MotionLibrary& pMotions, ItemAppearance& pAppearance)
    : mAssets(pAssets)
    , mMonsterCatalog(pMonsterCatalog)
    , mMotions(pMotions)
    , mAppearance(pAppearance)
{

}

void WorldEntities::setModel(WorldEntity& pEntity, std::shared_ptr<SkinnedModel> pModel)
{
    if (!pModel || pModel == pEntity.model)
    {
        return;
    }

    pEntity.model = std::move(pModel);
    pEntity.animator = std::make_unique<Animator>(*pEntity.model, &mMotions);
    pEntity.clips = AnimationRoles::resolve(*pEntity.model, &mMotions);

    if (pEntity.isDead)
    {
        playRole(pEntity, AnimationRoleType::eDeath, true, true);
    }
    else
    {
        playRole(pEntity, AnimationRoleType::eIdle, false);
    }
}

void WorldEntities::refreshBody(WorldEntity& pEntity)
{
    if (pEntity.type != EntityType::ePlayer)
    {
        return;
    }

    std::shared_ptr<SkinnedModel> tBody;

    for (const EquippedVisual& tEquipped : pEntity.equipment)
    {
        if (tEquipped.slot == EquipSlotType::eArmor)
        {
            if (const ItemLook* tLook = mAppearance.find(tEquipped.item))
            {
                tBody = tLook->model;
            }
        }
    }

    setModel(pEntity, tBody ? tBody : mAssets.getSkinnedModel(kPlayerBodyModel));

    if (pEntity.animator)
    {
        bool tIsArmed = std::any_of(pEntity.equipment.begin(), pEntity.equipment.end(),
                                    [](const EquippedVisual& pEquipped) { return pEquipped.slot == EquipSlotType::eWeapon; });
        const AnimationRoleClip& tGrip = pEntity.clips[static_cast<size_t>(AnimationRoleType::eGrip)];
        pEntity.animator->setGrip(tIsArmed ? tGrip.name : std::string(), tGrip.start);
    }
}

void WorldEntities::setLocalCharacterId(uint64_t pCharacterId)
{
    mLocalCharacterId = pCharacterId;
}

uint64_t WorldEntities::toLocalId(uint64_t pEntityId) const
{
    return pEntityId == mLocalCharacterId ? kLocalId : pEntityId;
}

WorldEntity* WorldEntities::find(uint64_t pEntityId)
{
    auto tIterator = mEntities.find(toLocalId(pEntityId));
    return tIterator == mEntities.end() ? nullptr : &tIterator->second;
}

const WorldEntity* WorldEntities::getLocal() const
{
    auto tIterator = mEntities.find(kLocalId);
    return tIterator == mEntities.end() ? nullptr : &tIterator->second;
}

void WorldEntities::playRole(WorldEntity& pEntity, AnimationRoleType pRole, bool pIsOnce, bool pHold)
{
    const AnimationRoleClip& tClip = pEntity.clips[static_cast<size_t>(pRole)];

    if (!pEntity.animator || tClip.name.empty())
    {
        return;
    }

    if (!pIsOnce)
    {
        pEntity.animator->play(tClip.name, 0.2f, tClip.rate, tClip.start, tClip.end);
    }
    else if (tClip.isSegment())
    {
        pEntity.animator->playSegment(tClip.name, tClip.start, tClip.end, static_cast<float>(WorldView::kAttackSegmentSeconds));
    }
    else
    {
        pEntity.animator->playOnce(tClip.name, 0.1f, pHold);
    }
}

void WorldEntities::applyDeath(WorldEntity& pEntity, bool pIsDead)
{
    if (pIsDead == pEntity.isDead)
    {
        return;
    }

    pEntity.isDead = pIsDead;
    playRole(pEntity, pIsDead ? AnimationRoleType::eDeath : AnimationRoleType::eIdle, pIsDead, pIsDead);
}

void WorldEntities::playLocalAttack(uint32_t pCombo)
{
    auto tIterator = mEntities.find(kLocalId);

    if (tIterator != mEntities.end())
    {
        playRole(tIterator->second, static_cast<AnimationRoleType>(static_cast<size_t>(AnimationRoleType::eAttack1) + pCombo % 3), true);
    }
}

glm::vec2 WorldEntities::takeLocalRootMotion()
{
    auto tIterator = mEntities.find(kLocalId);

    if (tIterator == mEntities.end() || !tIterator->second.animator || !tIterator->second.model)
    {
        return glm::vec2(0.0f);
    }

    const WorldEntity& tLocal = tIterator->second;
    glm::vec3 tShift = tIterator->second.animator->takeRootMotion() * (WorldView::kPlayerBoxHalfExtents.y * 2.0f / tLocal.model->getHeight());
    float tCos = std::cos(tLocal.facing);
    float tSin = std::sin(tLocal.facing);
    return glm::vec2(tShift.x * tCos + tShift.z * tSin, tShift.z * tCos - tShift.x * tSin);
}

std::vector<DamageEvent> WorldEntities::takeDamageEvents()
{
    return std::exchange(mDamageEvents, {});
}

std::shared_ptr<SkinnedModel> WorldEntities::loadMonsterModel(const std::string& pModelPath)
{
    std::shared_ptr<SkinnedModel> tModel = pModelPath.empty() ? nullptr : mAssets.getSkinnedModel(pModelPath);

    if (tModel)
    {
        return tModel;
    }

    tModel = mAssets.getSkinnedModel(kMonsterPlaceholderModel);

    if (!tModel)
    {
        tModel = mAssets.getSkinnedModel(kMonsterFallbackModel);
    }

    if (mFallbackLogged.insert(pModelPath).second)
    {
        SDL_Log("WorldEntities: monster model '%s' unavailable, using %s", pModelPath.c_str(), tModel ? "fallback model" : "no model");
    }

    return tModel;
}

WorldEntity& WorldEntities::spawn(uint64_t pId, EntityType pType, const std::string& pName, const glm::vec3& pPosition,
                                  const std::string& pModelPath)
{
    WorldEntity tEntity;
    tEntity.id = pId;
    tEntity.type = pType;
    tEntity.name = pName;
    tEntity.previousPosition = pPosition;
    tEntity.targetPosition = pPosition;
    tEntity.interpolationDuration = WorldView::kStateSendInterval;
    tEntity.interpolationElapsed = tEntity.interpolationDuration;
    tEntity.lastAnimatedPosition = pPosition;

    WorldEntity& tStored = mEntities.insert_or_assign(pId, std::move(tEntity)).first->second;
    setModel(tStored, pType == EntityType::eMonster ? loadMonsterModel(pModelPath) : mAssets.getSkinnedModel(pModelPath.empty() ? kPlayerBodyModel : pModelPath));
    return tStored;
}

void WorldEntities::applySnapshot(const WorldController::WorldSnapshot& pSnapshot)
{
    for (const auto& tEntity : pSnapshot.entered)
    {
        if (tEntity.kind == 3)
        {
            WorldEntity tGround;
            tGround.id = tEntity.entityId;
            tGround.type = EntityType::eGroundItem;
            tGround.itemTemplateId = tEntity.templateId;
            tGround.itemCount = tEntity.count;
            tGround.gold = tEntity.gold;
            tGround.name = tEntity.gold > 0 ? std::to_string(tEntity.gold) + " Gold"
                         : tEntity.count > 1 ? tEntity.name + " x" + std::to_string(tEntity.count) : tEntity.name;
            tGround.previousPosition = tGround.targetPosition = tGround.lastAnimatedPosition = glm::vec3(tEntity.x, tEntity.y, tEntity.z);
            tGround.interpolationElapsed = tGround.interpolationDuration;
            mEntities.insert_or_assign(tEntity.entityId, std::move(tGround));
            continue;
        }

        EntityType tType = tEntity.kind == 1 ? EntityType::eMonster : tEntity.kind == 2 ? EntityType::eNpc : EntityType::ePlayer;
        std::string tModel = tEntity.model;
        const MonsterTemplate* tMonsterTemplate = nullptr;

        if (tType == EntityType::eMonster)
        {
            tMonsterTemplate = mMonsterCatalog.find(tEntity.templateId);

            if (tMonsterTemplate)
            {
                tModel = tMonsterTemplate->model;
            }
        }

        WorldEntity& tSpawned = spawn(tEntity.entityId, tType, tEntity.name, glm::vec3(tEntity.x, tEntity.y, tEntity.z), tModel);
        tSpawned.level = tEntity.level;

        if (tMonsterTemplate)
        {
            tSpawned.tint = glm::vec3(tMonsterTemplate->tint[0], tMonsterTemplate->tint[1], tMonsterTemplate->tint[2]);
            tSpawned.scale = tMonsterTemplate->scale;
        }

        tSpawned.equipment = tEntity.equipment;
        tSpawned.kingdom = tEntity.kingdom;
        tSpawned.healthPercent = tEntity.healthPercent;
        applyDeath(tSpawned, tEntity.isDead);
        refreshBody(tSpawned);
    }

    for (const auto& tEntity : pSnapshot.moved)
    {
        auto tIterator = mEntities.find(tEntity.entityId);

        if (tIterator == mEntities.end())
        {
            continue;
        }

        WorldEntity& tExisting = tIterator->second;
        tExisting.previousPosition = getPosition(tExisting);
        tExisting.targetPosition = glm::vec3(tEntity.x, tEntity.y, tEntity.z);
        tExisting.interpolationElapsed = 0.0;
    }

    for (const auto& tAppearance : pSnapshot.appearance)
    {
        auto tIterator = mEntities.find(tAppearance.entityId);

        if (tIterator != mEntities.end())
        {
            tIterator->second.equipment = tAppearance.equipment;
            refreshBody(tIterator->second);
        }
    }

    for (const auto& tEvent : pSnapshot.combat)
    {
        uint64_t tAttackerId = toLocalId(tEvent.attackerId);
        WorldEntity* tAttacker = find(tEvent.attackerId);

        if (tAttacker && tAttackerId != kLocalId)
        {
            playRole(*tAttacker, static_cast<AnimationRoleType>(static_cast<size_t>(AnimationRoleType::eAttack1) + tEvent.combo % 3), true);
        }

        for (const auto& tHit : tEvent.hits)
        {
            uint64_t tTargetId = toLocalId(tHit.targetId);
            mDamageEvents.push_back({ tTargetId, tHit.damage, tTargetId == kLocalId, tAttackerId == kLocalId });

            if (WorldEntity* tTarget = find(tHit.targetId); tTarget && tTarget->animator && !tHit.isKilled && !tTarget->animator->isBusy())
            {
                playRole(*tTarget, AnimationRoleType::eHurt, true);
            }
        }
    }

    for (const auto& tVitals : pSnapshot.vitals)
    {
        if (WorldEntity* tEntity = find(tVitals.entityId))
        {
            tEntity->healthPercent = tVitals.healthPercent;
            applyDeath(*tEntity, tVitals.isDead);
        }
    }

    for (uint64_t tEntityId : pSnapshot.left)
    {
        mEntities.erase(tEntityId);
    }
}

void WorldEntities::clearRemote()
{
    std::erase_if(mEntities, [](const auto& pEntry) { return pEntry.first != kLocalId; });
}

void WorldEntities::setLocal(const std::string& pName, const glm::vec3& pPosition, double pStepSeconds)
{
    auto tIterator = mEntities.find(kLocalId);

    if (tIterator == mEntities.end())
    {
        spawn(kLocalId, EntityType::ePlayer, pName, pPosition);
        return;
    }

    WorldEntity& tLocal = tIterator->second;
    tLocal.previousPosition = tLocal.targetPosition;
    tLocal.targetPosition = pPosition;
    tLocal.interpolationElapsed = 0.0;
    tLocal.interpolationDuration = pStepSeconds;
}

void WorldEntities::teleportLocal(const glm::vec3& pPosition)
{
    auto tIterator = mEntities.find(kLocalId);

    if (tIterator != mEntities.end())
    {
        tIterator->second.previousPosition = pPosition;
        tIterator->second.targetPosition = pPosition;
        tIterator->second.lastAnimatedPosition = pPosition;
    }
}

void WorldEntities::setLocalEquipment(const std::vector<OwnedItem>& pItems)
{
    auto tIterator = mEntities.find(kLocalId);

    if (tIterator == mEntities.end())
    {
        return;
    }

    std::vector<EquippedVisual>& tEquipment = tIterator->second.equipment;
    tEquipment.clear();

    for (const OwnedItem& tItem : pItems)
    {
        if (tItem.location == ItemLocationType::eEquipped)
        {
            tEquipment.push_back({ static_cast<EquipSlotType>(tItem.slot), { tItem.templateId, 0 } });
        }
    }
}

void WorldEntities::showSpeech(uint64_t pId, const std::string& pText)
{
    auto tIterator = mEntities.find(pId);

    if (tIterator != mEntities.end())
    {
        tIterator->second.speechText = pText;
        tIterator->second.speechRemainingSeconds = WorldView::kSpeechBubbleVisibleSeconds;
    }
}

void WorldEntities::update(double pDeltaTime)
{
    for (auto& [tId, tEntity] : mEntities)
    {
        refreshBody(tEntity);
        tEntity.interpolationElapsed += pDeltaTime;

        if (tEntity.speechRemainingSeconds > 0.0)
        {
            tEntity.speechRemainingSeconds -= pDeltaTime;

            if (tEntity.speechRemainingSeconds <= 0.0)
            {
                tEntity.speechText.clear();
            }
        }

        animate(tEntity, pDeltaTime);
    }
}

void WorldEntities::animate(WorldEntity& pEntity, double pDeltaTime)
{
    if (!pEntity.animator)
    {
        return;
    }

    if (pEntity.isDead || pEntity.animator->isBusy())
    {
        pEntity.lastAnimatedPosition = getPosition(pEntity);
        pEntity.animator->update(static_cast<float>(pDeltaTime));
        return;
    }

    glm::vec3 tPosition = getPosition(pEntity);
    glm::vec2 tStep(tPosition.x - pEntity.lastAnimatedPosition.x, tPosition.z - pEntity.lastAnimatedPosition.z);
    float tSpeed = pDeltaTime > 0.0 ? glm::length(tStep) / static_cast<float>(pDeltaTime) : 0.0f;

    if (tSpeed > kMovingSpeed)
    {
        pEntity.movingTimer = kMovingHoldSeconds;
        pEntity.facing = turnTowards(pEntity.facing, std::atan2(tStep.x, tStep.y), kTurnSpeed * static_cast<float>(pDeltaTime));
    }
    else
    {
        pEntity.movingTimer -= pDeltaTime;
    }

    pEntity.lastAnimatedPosition = tPosition;

    playRole(pEntity, pEntity.movingTimer > 0.0 ? AnimationRoleType::eRun : AnimationRoleType::eIdle, false);
    pEntity.animator->update(static_cast<float>(pDeltaTime));
}

const std::unordered_map<uint64_t, WorldEntity>& WorldEntities::getAll() const
{
    return mEntities;
}

std::optional<glm::vec3> WorldEntities::findPosition(uint64_t pId) const
{
    auto tIterator = mEntities.find(pId);

    if (tIterator == mEntities.end())
    {
        return std::nullopt;
    }

    return getPosition(tIterator->second);
}

glm::vec3 WorldEntities::getPosition(const WorldEntity& pEntity, double pExtraSeconds)
{
    double tDuration = pEntity.interpolationDuration > 0.0 ? pEntity.interpolationDuration : 1.0;
    float tProgress = static_cast<float>(std::min((pEntity.interpolationElapsed + pExtraSeconds) / tDuration, 1.0));
    return glm::mix(pEntity.previousPosition, pEntity.targetPosition, tProgress);
}
