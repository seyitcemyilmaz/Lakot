#include "WorldEffects.h"

#include <SDL3/SDL_log.h>

#include "WorldEntities.h"
#include "../../asset/AssetManager.h"

using namespace lakot;

namespace
{
    EffectCatalog loadCatalog(const AssetManager& pAssets)
    {
        EffectCatalog tCatalog;
        std::string tError;

        if (!tCatalog.load(pAssets.resolve("effects.json"), tError))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "EffectCatalog: %s", tError.c_str());
            return EffectCatalog();
        }

        SDL_Log("EffectCatalog: %zu effects, %zu textures", tCatalog.getEffects().size(), tCatalog.getTextureNames().size());
        return tCatalog;
    }
}

WorldEffects::WorldEffects(AssetManager& pAssets, ShaderManager& pShaders, const WorldEntities& pEntities, const DisplaySettings& pDisplaySettings, uint32_t pInitialLevel)
    : mEntities(pEntities)
    , mDisplaySettings(pDisplaySettings)
    , mSystem(loadCatalog(pAssets))
    , mRenderer(pShaders, pAssets)
    , mLastLevel(pInitialLevel)
{
    mRenderer.loadTextures(mSystem.getCatalog());
}

void WorldEffects::onSnapshot(const WorldController::WorldSnapshot& pSnapshot)
{
    for (const WorldController::CombatSnapshot& tCombat : pSnapshot.combat)
    {
        onHits(tCombat);
    }

    for (const WorldController::EntitySnapshot& tEntity : pSnapshot.entered)
    {
        if (tEntity.kind == 3)
        {
            onLoot(tEntity);
        }
    }

    if (pSnapshot.selfVitals)
    {
        if (pSnapshot.selfVitals->level > mLastLevel)
        {
            mSystem.attach(kLevelUp, WorldEntities::kLocalId, glm::vec3(0.0f, -WorldView::kPlayerBoxHalfExtents.y, 0.0f));
        }

        mLastLevel = pSnapshot.selfVitals->level;
    }
}

void WorldEffects::onHits(const WorldController::CombatSnapshot& pCombat)
{
    for (const WorldController::HitSnapshot& tHit : pCombat.hits)
    {
        auto tPosition = mEntities.findServerEntityPosition(tHit.targetId);

        if (!tPosition || tHit.damage == 0)
        {
            continue;
        }

        glm::vec3 tChest = *tPosition + glm::vec3(0.0f, WorldView::kPlayerBoxHalfExtents.y, 0.0f);
        spawnNearLocal(kHitSpark, tChest);

        if (tHit.isKilled)
        {
            spawnNearLocal(kHitKill, tChest);
            mKills.push_back({ *tPosition, mTime });
        }
    }
}

void WorldEffects::onLoot(const WorldController::EntitySnapshot& pEntity)
{
    glm::vec3 tPosition(pEntity.x, pEntity.y, pEntity.z);

    bool tIsKillDrop = isNearRecentKill(tPosition);
    bool tIsSelfDrop = mLootGraceRemaining <= 0.0 && isNearLocal(tPosition, kSelfLootDistance);

    if (tIsKillDrop || tIsSelfDrop)
    {
        spawnNearLocal(pEntity.gold > 0 ? kLootDropGold : kLootDrop, tPosition);
    }
}

bool WorldEffects::isNearRecentKill(const glm::vec3& pPosition) const
{
    for (const KillRecord& tKill : mKills)
    {
        if (mTime - tKill.time <= kKillMemorySeconds && glm::distance(tKill.position, pPosition) <= kKillLootDistance)
        {
            return true;
        }
    }

    return false;
}

bool WorldEffects::isNearLocal(const glm::vec3& pPosition, float pDistance) const
{
    const WorldEntity* tLocal = mEntities.getLocal();
    return tLocal && glm::distance(WorldEntities::getPosition(*tLocal), pPosition) <= pDistance;
}

void WorldEffects::spawnNearLocal(const char* pName, const glm::vec3& pPosition)
{
    if (const WorldEntity* tLocal = mEntities.getLocal(); tLocal && glm::distance(WorldEntities::getPosition(*tLocal), pPosition) > mDisplaySettings.getViewDistance())
    {
        return;
    }

    mSystem.spawn(pName, pPosition);
}

void WorldEffects::spawnAtEntity(const std::string& pName, uint64_t pEntityId)
{
    auto tFound = mEntities.getAll().find(pEntityId);

    if (tFound != mEntities.getAll().end())
    {
        spawnNearLocal(pName.c_str(), WorldEntities::getPosition(tFound->second) - glm::vec3(0.0f, WorldView::kPlayerBoxHalfExtents.y, 0.0f));
    }
}

void WorldEffects::update(double pDeltaSeconds)
{
    mTime += pDeltaSeconds;
    mLootGraceRemaining -= pDeltaSeconds;

    std::erase_if(mKills, [this](const KillRecord& pKill)
    {
        return mTime - pKill.time > kKillMemorySeconds;
    });

    mSystem.update(static_cast<float>(pDeltaSeconds), mEntities);
}

void WorldEffects::render(const Camera& pCamera, const glm::vec3& pFogColor, float pFogStart, float pFogEnd, double pExtraSeconds)
{
    mRenderer.render(mSystem, pCamera, pFogColor, pFogStart, pFogEnd, pExtraSeconds);
}

void WorldEffects::clear()
{
    mSystem.clear();
    mKills.clear();
    mLootGraceRemaining = kLootGraceSeconds;
}

bool WorldEffects::reload(const std::string& pPath, std::string& pError)
{
    if (!mSystem.reload(pPath, pError))
    {
        return false;
    }

    mRenderer.loadTextures(mSystem.getCatalog());
    return true;
}

EffectSystem& WorldEffects::getSystem()
{
    return mSystem;
}

const ParticleRenderer& WorldEffects::getRenderer() const
{
    return mRenderer;
}
