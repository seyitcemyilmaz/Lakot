#include "MapAssetPrefetcher.h"

#include <cmath>
#include <filesystem>
#include <string>

#include "MapCatalog.h"
#include "MonsterCatalog.h"

#include "MapGroundPreparer.h"

#include "../../asset/AssetManager.h"

using namespace lakot;

MapAssetPrefetcher::MapAssetPrefetcher(AssetManager& pAssets, const MapCatalog& pMaps, const MonsterCatalog& pMonsters, MapGroundPreparer& pGround)
    : mAssets(pAssets)
    , mMaps(pMaps)
    , mMonsters(pMonsters)
    , mGround(pGround)
{

}

void MapAssetPrefetcher::prefetchMap(uint32_t pMapId)
{
    const MapData* tMap = mMaps.getMap(pMapId);

    if (!tMap)
    {
        return;
    }

    bool tIsGroundNew = mGroundRequested.insert(pMapId).second;

    if (tIsGroundNew)
    {
        mGround.requestGeometry(*tMap);
    }

    std::chrono::steady_clock::time_point tNow = std::chrono::steady_clock::now();
    auto tPrefetched = mPrefetchedMaps.find(pMapId);

    if (tPrefetched != mPrefetchedMaps.end() && tNow - tPrefetched->second < kRefreshInterval)
    {
        return;
    }

    mPrefetchedMaps.insert_or_assign(pMapId, tNow);

    std::unordered_set<std::string> tModels;

    for (const MapObject& tObject : tMap->getObjects())
    {
        if (tObject.model.empty() || !tModels.insert(tObject.model).second)
        {
            continue;
        }

        mAssets.prefetchModel("models/" + tObject.model + ".glb");

        std::string tLodPath = "models/" + tObject.model + "_lod1.glb";

        if (std::filesystem::exists(mAssets.resolve(tLodPath)))
        {
            mAssets.prefetchModel(tLodPath);
        }
    }

    for (const MapNpc& tNpc : tMap->getNpcs())
    {
        if (!tNpc.model.empty())
        {
            mAssets.prefetchSkinnedModel(tNpc.model);
        }
    }

    for (const MapMonsterSpawn& tSpawn : tMap->getMonsterSpawns())
    {
        const MonsterTemplate* tTemplate = mMonsters.find(tSpawn.monsterId);

        if (tTemplate && !tTemplate->model.empty())
        {
            mAssets.prefetchSkinnedModel(tTemplate->model);
        }
    }

    if (tIsGroundNew)
    {
        mGround.requestImage(*tMap, kMapImageSize);
    }
}

void MapAssetPrefetcher::prefetchNearbyPortalTargets(const MapData& pCurrent, float pX, float pZ)
{
    for (const MapPortal& tPortal : pCurrent.getPortals())
    {
        float tDistance = std::hypot(tPortal.triggerX - pX, tPortal.triggerZ - pZ) - tPortal.triggerRadius;

        if (tDistance <= kPortalLookaheadMeters)
        {
            prefetchMap(tPortal.targetMapId);
        }
    }
}

void MapAssetPrefetcher::reset()
{
    mPrefetchedMaps.clear();
    mGroundRequested.clear();
}
