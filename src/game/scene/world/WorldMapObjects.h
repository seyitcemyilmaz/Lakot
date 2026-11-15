#ifndef LAKOT_WORLD_MAP_OBJECTS_H
#define LAKOT_WORLD_MAP_OBJECTS_H

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "MapData.h"

#include "MapModelBatch.h"

#include "../../graphics/geometry/BoxContainer.h"

namespace lakot
{

class AssetManager;

class WorldMapObjects
{
public:
    explicit WorldMapObjects(AssetManager& pAssets);

    void load(const MapData& pMap);
    void update();

    std::vector<MapModelBatch>& getBatches();
    BoxContainer& getBoxes();

private:
    struct ModelGroup
    {
        std::string name;
        std::vector<const MapObject*> objects;
    };

    static constexpr double kFrameBudgetSeconds = 0.004;

    AssetManager& mAssets;
    BoxContainer mBoxes;
    const MapData* mMap{nullptr};

    std::vector<MapModelBatch> mBatches;
    std::vector<ModelGroup> mPending;

    std::vector<glm::vec3> mBoxPositions;
    std::vector<glm::vec3> mBoxSizes;
    std::vector<glm::vec3> mBoxColors;
    bool mAreBoxesDirty{false};

    void resolvePending(double pBudgetSeconds);
    bool tryBuildBatch(const ModelGroup& pGroup);
    void addBox(const MapObject& pObject);
    void addWater();
    void uploadBoxes();
};

}

#endif
