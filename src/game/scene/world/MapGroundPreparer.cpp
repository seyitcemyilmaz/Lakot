#include "MapGroundPreparer.h"

#include <condition_variable>
#include <mutex>
#include <unordered_map>

#include "GarbageCollector.h"

#include "../../graphics/geometry/TerrainImageLoader.h"
#include "../../graphics/geometry/TerrainMeshFactory.h"

using namespace lakot;

struct MapGroundPreparer::State
{
    std::mutex mutex;
    std::optional<TerrainLayerImages> layers;
    std::unordered_map<uint32_t, MapGeometry> geometry;
    std::unordered_map<uint32_t, MapImage> images;
    std::condition_variable idle;
    bool isCancelled{false};
    int runningCount{0};

    bool tryBegin()
    {
        std::lock_guard<std::mutex> tLock(mutex);

        if (isCancelled)
        {
            return false;
        }

        ++runningCount;
        return true;
    }

    template <typename T>
    void finish(std::unordered_map<uint32_t, T>& pTarget, uint32_t pMapId, T&& pValue)
    {
        std::lock_guard<std::mutex> tLock(mutex);
        pTarget.emplace(pMapId, std::move(pValue));
        --runningCount;
        idle.notify_all();
    }
};

MapGroundPreparer::MapGroundPreparer(GarbageCollector& pGarbageCollector)
    : mGarbageCollector(pGarbageCollector)
    , mState(std::make_shared<State>())
{

}

MapGroundPreparer::~MapGroundPreparer()
{
    std::unique_lock<std::mutex> tLock(mState->mutex);
    mState->isCancelled = true;
    mState->idle.wait(tLock, [this]() { return mState->runningCount == 0; });
}

void MapGroundPreparer::requestLayers(const std::string& pTextureDirectory)
{
    if (mAreLayersRequested)
    {
        return;
    }

    mAreLayersRequested = true;
    std::shared_ptr<State> tState = mState;

    mGarbageCollector.addAsynchronousTask([tState, pTextureDirectory]()
    {
        if (!tState->tryBegin())
        {
            return;
        }

        TerrainLayerImages tLayers = TerrainImageLoader::loadLayers(pTextureDirectory);

        std::lock_guard<std::mutex> tLock(tState->mutex);
        tState->layers = std::move(tLayers);
        --tState->runningCount;
        tState->idle.notify_all();
    });
}

void MapGroundPreparer::requestGeometry(const MapData& pMap)
{
    if (!mRequestedGeometry.insert(pMap.getId()).second)
    {
        return;
    }

    std::shared_ptr<State> tState = mState;
    const MapData* tMap = &pMap;

    mGarbageCollector.addAsynchronousTask([tState, tMap]()
    {
        if (!tState->tryBegin())
        {
            return;
        }

        MapGeometry tGeometry;
        tGeometry.mesh = TerrainMeshFactory::create(*tMap);
        tGeometry.splat = TerrainImageLoader::loadSplat(tMap->getDirectory());
        tState->finish(tState->geometry, tMap->getId(), std::move(tGeometry));
    });
}

void MapGroundPreparer::requestImage(const MapData& pMap, int pSize)
{
    if (!mRequestedImages.insert(pMap.getId()).second)
    {
        return;
    }

    std::shared_ptr<State> tState = mState;
    const MapData* tMap = &pMap;

    mGarbageCollector.addAsynchronousTask([tState, tMap, pSize]()
    {
        if (!tState->tryBegin())
        {
            return;
        }

        MapImage tImage = MapImageFactory::create(*tMap, pSize);
        tState->finish(tState->images, tMap->getId(), std::move(tImage));
    });
}

std::optional<TerrainLayerImages> MapGroundPreparer::takeLayers()
{
    std::lock_guard<std::mutex> tLock(mState->mutex);
    std::optional<TerrainLayerImages> tResult = std::move(mState->layers);
    mState->layers.reset();
    return tResult;
}

std::optional<MapGeometry> MapGroundPreparer::takeGeometry(uint32_t pMapId)
{
    std::lock_guard<std::mutex> tLock(mState->mutex);
    auto tFound = mState->geometry.find(pMapId);

    if (tFound == mState->geometry.end())
    {
        return std::nullopt;
    }

    std::optional<MapGeometry> tResult(std::move(tFound->second));
    mState->geometry.erase(tFound);
    mRequestedGeometry.erase(pMapId);
    return tResult;
}

std::optional<MapImage> MapGroundPreparer::takeImage(uint32_t pMapId)
{
    std::lock_guard<std::mutex> tLock(mState->mutex);
    auto tFound = mState->images.find(pMapId);

    if (tFound == mState->images.end())
    {
        return std::nullopt;
    }

    std::optional<MapImage> tResult(std::move(tFound->second));
    mState->images.erase(tFound);
    mRequestedImages.erase(pMapId);
    return tResult;
}
