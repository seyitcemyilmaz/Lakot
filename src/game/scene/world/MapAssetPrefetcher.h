#ifndef LAKOT_MAP_ASSET_PREFETCHER_H
#define LAKOT_MAP_ASSET_PREFETCHER_H

#include <cstdint>
#include <chrono>
#include <unordered_map>
#include <unordered_set>

namespace lakot
{

class AssetManager;
class MapCatalog;
class MapData;
class MapGroundPreparer;
class MonsterCatalog;

class MapAssetPrefetcher
{
public:
    MapAssetPrefetcher(AssetManager& pAssets, const MapCatalog& pMaps, const MonsterCatalog& pMonsters, MapGroundPreparer& pGround);

    void prefetchMap(uint32_t pMapId);
    void prefetchNearbyPortalTargets(const MapData& pCurrent, float pX, float pZ);
    void reset();

private:
    static constexpr int kMapImageSize = 1024;
    static constexpr float kPortalLookaheadMeters = 250.0f;
    static constexpr std::chrono::seconds kRefreshInterval{60};

    AssetManager& mAssets;
    const MapCatalog& mMaps;
    const MonsterCatalog& mMonsters;
    MapGroundPreparer& mGround;

    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> mPrefetchedMaps;
    std::unordered_set<uint32_t> mGroundRequested;
};

}

#endif
