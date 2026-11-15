#ifndef LAKOT_MAP_GROUND_PREPARER_H
#define LAKOT_MAP_GROUND_PREPARER_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>

#include "MapData.h"

#include "../../graphics/geometry/TerrainLayerImages.h"
#include "../../graphics/geometry/TerrainMesh.h"
#include "../../gui/MapImageFactory.h"

namespace lakot
{

class GarbageCollector;

struct MapGeometry
{
    TerrainMesh mesh;
    DecodedImage splat;
};

class MapGroundPreparer
{
public:
    explicit MapGroundPreparer(GarbageCollector& pGarbageCollector);
    ~MapGroundPreparer();

    MapGroundPreparer(const MapGroundPreparer&) = delete;
    MapGroundPreparer& operator=(const MapGroundPreparer&) = delete;

    void requestLayers(const std::string& pTextureDirectory);
    void requestGeometry(const MapData& pMap);
    void requestImage(const MapData& pMap, int pSize);

    std::optional<TerrainLayerImages> takeLayers();
    std::optional<MapGeometry> takeGeometry(uint32_t pMapId);
    std::optional<MapImage> takeImage(uint32_t pMapId);

private:
    struct State;

    GarbageCollector& mGarbageCollector;
    std::shared_ptr<State> mState;
    bool mAreLayersRequested{false};
    std::unordered_set<uint32_t> mRequestedGeometry;
    std::unordered_set<uint32_t> mRequestedImages;
};

}

#endif
