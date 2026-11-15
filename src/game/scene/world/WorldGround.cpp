#include "WorldGround.h"

#include <utility>
#include <vector>

#include "MapData.h"

#include "../../gui/rml/RmlUiLayer.h"
#include "../../graphics/render/ShaderProgram.h"

using namespace lakot;

WorldGround::WorldGround(GarbageCollector& pGarbageCollector, RmlUiLayer& pRmlUiLayer, const std::string& pTextureDirectory, ImageReadyCallback pImageReadyCallback)
    : mRmlUiLayer(pRmlUiLayer)
    , mImageReadyCallback(std::move(pImageReadyCallback))
    , mPreparer(pGarbageCollector)
{
    mPreparer.requestLayers(pTextureDirectory);
    mRmlUiLayer.registerGeneratedImage(kPlaceholderImageName, 1, 1, std::vector<uint8_t>(4, 0));
}

MapGroundPreparer& WorldGround::getPreparer()
{
    return mPreparer;
}

std::string WorldGround::makeImageName(uint32_t pMapId)
{
    return "map-" + std::to_string(pMapId);
}

std::string WorldGround::setMap(const MapData& pMap)
{
    mMap = &pMap;
    mTerrain.reset();
    mIsGeometryPending = true;
    mIsImagePending = true;
    return kPlaceholderImageName;
}

void WorldGround::update()
{
    if (updateLayers() || updateGeometry())
    {
        return;
    }

    updateImage();
}

bool WorldGround::updateLayers()
{
    if (!mAreLayersPending)
    {
        return false;
    }

    std::optional<TerrainLayerImages> tLayers = mPreparer.takeLayers();

    if (!tLayers)
    {
        return false;
    }

    mAreLayersPending = false;
    mMaterial.setLayers(*tLayers);
    return true;
}

bool WorldGround::updateGeometry()
{
    if (!mIsGeometryPending || mAreLayersPending)
    {
        return false;
    }

    std::optional<MapGeometry> tGeometry = mPreparer.takeGeometry(mMap->getId());

    if (!tGeometry)
    {
        return false;
    }

    mIsGeometryPending = false;
    mMaterial.setSplat(tGeometry->splat);
    mTerrain = std::make_unique<Terrain>(std::move(tGeometry->mesh));
    mTerrain->initialize();
    return true;
}

bool WorldGround::updateImage()
{
    if (!mIsImagePending)
    {
        return false;
    }

    std::optional<MapImage> tImage = mPreparer.takeImage(mMap->getId());

    if (!tImage)
    {
        return false;
    }

    mIsImagePending = false;

    std::string tName = makeImageName(mMap->getId());
    mRmlUiLayer.registerGeneratedImage(tName, tImage->size, tImage->size, std::move(tImage->rgba));
    mImageReadyCallback(tName);
    return true;
}

void WorldGround::draw(ShaderProgram& pShader, const glm::vec3& pCenter, float pMaxDistance) const
{
    if (!mTerrain || !mMap)
    {
        return;
    }

    mMaterial.bind(pShader, mMap->getWorldSize());
    mTerrain->getVertexArrayObject().bind();
    mTerrain->drawChunksNear(pCenter, pMaxDistance);
}
