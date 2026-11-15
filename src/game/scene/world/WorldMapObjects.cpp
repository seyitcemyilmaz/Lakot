#include "WorldMapObjects.h"

#include <chrono>
#include <limits>
#include <limits>
#include <filesystem>
#include <memory>
#include <unordered_map>

#include "WorldPicker.h"
#include "WorldViewConstants.h"

#include "../../asset/AssetManager.h"
#include "../../graphics/model/Model.h"

using namespace lakot;

namespace
{
    const glm::vec3 kWaterColor(0.20f, 0.42f, 0.62f);
    constexpr float kWaterThickness = 0.1f;
}

WorldMapObjects::WorldMapObjects(AssetManager& pAssets)
    : mAssets(pAssets)
{
    mBoxes.initialize();
}

std::vector<MapModelBatch>& WorldMapObjects::getBatches()
{
    return mBatches;
}

BoxContainer& WorldMapObjects::getBoxes()
{
    return mBoxes;
}

void WorldMapObjects::load(const MapData& pMap)
{
    mMap = &pMap;
    mBatches.clear();
    mPending.clear();
    mBoxPositions.clear();
    mBoxSizes.clear();
    mBoxColors.clear();

    std::unordered_map<std::string, size_t> tGroupIndices;

    for (const MapObject& tObject : pMap.getObjects())
    {
        if (tObject.model.empty())
        {
            addBox(tObject);
            continue;
        }

        auto tGroup = tGroupIndices.find(tObject.model);

        if (tGroup == tGroupIndices.end())
        {
            tGroup = tGroupIndices.emplace(tObject.model, mPending.size()).first;
            mPending.push_back(ModelGroup{ tObject.model, {} });
        }

        mPending[tGroup->second].objects.push_back(&tObject);
    }

    addWater();
    uploadBoxes();
    resolvePending(std::numeric_limits<double>::infinity());
}

void WorldMapObjects::update()
{
    if (!mPending.empty())
    {
        resolvePending(kFrameBudgetSeconds);
    }

    if (mAreBoxesDirty)
    {
        uploadBoxes();
    }
}

void WorldMapObjects::resolvePending(double pBudgetSeconds)
{
    std::chrono::steady_clock::time_point tStart = std::chrono::steady_clock::now();

    for (size_t tIndex = 0; tIndex < mPending.size();)
    {
        if (!tryBuildBatch(mPending[tIndex]))
        {
            ++tIndex;
            continue;
        }

        mPending.erase(mPending.begin() + static_cast<std::ptrdiff_t>(tIndex));

        if (std::chrono::duration<double>(std::chrono::steady_clock::now() - tStart).count() >= pBudgetSeconds)
        {
            break;
        }
    }
}

bool WorldMapObjects::tryBuildBatch(const ModelGroup& pGroup)
{
    std::shared_ptr<Model> tModel;
    std::shared_ptr<Model> tLodModel;

    if (!mAssets.tryGetModel("models/" + pGroup.name + ".glb", tModel))
    {
        return false;
    }

    std::string tLodPath = "models/" + pGroup.name + "_lod1.glb";

    if (tModel && std::filesystem::exists(mAssets.resolve(tLodPath)) && !mAssets.tryGetModel(tLodPath, tLodModel))
    {
        return false;
    }

    if (!tModel)
    {
        for (const MapObject* tObject : pGroup.objects)
        {
            addBox(*tObject);
        }

        mAreBoxesDirty = true;
        return true;
    }

    MapModelBatch tBatch(tModel, tLodModel);

    if (pGroup.objects.front()->isUndergrowth())
    {
        tBatch.setDrawDistance(WorldView::kUndergrowthDrawDistance);
    }

    for (const MapObject* tObject : pGroup.objects)
    {
        float tGround = mMap->getHeightAt(tObject->x, tObject->z);
        tBatch.addInstance(glm::vec3(tObject->x, tGround + tObject->elevation, tObject->z),
                           glm::vec3(tObject->yaw, tObject->scale, tObject->stretch));
    }

    mBatches.push_back(std::move(tBatch));
    return true;
}

void WorldMapObjects::addBox(const MapObject& pObject)
{
    if (pObject.isHidden || pObject.isUndergrowth())
    {
        return;
    }

    mBoxPositions.push_back(WorldPicker::getObjectCenter(*mMap, pObject));
    mBoxSizes.emplace_back(pObject.sizeX, pObject.sizeY, pObject.sizeZ);
    mBoxColors.emplace_back(pObject.color[0], pObject.color[1], pObject.color[2]);
}

void WorldMapObjects::addWater()
{
    const float tWorldSize = mMap->getWorldSize();

    mBoxPositions.emplace_back(0.0f, mMap->getWaterLevel() - kWaterThickness * 0.5f, 0.0f);
    mBoxSizes.emplace_back(tWorldSize, kWaterThickness, tWorldSize);
    mBoxColors.push_back(kWaterColor);
}

void WorldMapObjects::uploadBoxes()
{
    mBoxes.setBoxes(mBoxPositions, mBoxSizes, mBoxColors);
    mAreBoxesDirty = false;
}
