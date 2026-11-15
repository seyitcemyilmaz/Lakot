#ifndef LAKOT_WORLD_GROUND_H
#define LAKOT_WORLD_GROUND_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <glm/glm.hpp>

#include "MapGroundPreparer.h"

#include "../../graphics/geometry/Terrain.h"
#include "../../graphics/geometry/TerrainMaterial.h"

namespace lakot
{

class GarbageCollector;
class MapData;
class RmlUiLayer;
class ShaderProgram;

class WorldGround
{
public:
    using ImageReadyCallback = std::function<void(const std::string& pImageName)>;

    WorldGround(GarbageCollector& pGarbageCollector, RmlUiLayer& pRmlUiLayer, const std::string& pTextureDirectory, ImageReadyCallback pImageReadyCallback);

    MapGroundPreparer& getPreparer();

    std::string setMap(const MapData& pMap);
    void update();
    void draw(ShaderProgram& pShader, const glm::vec3& pCenter, float pMaxDistance) const;

private:
    static constexpr const char* kPlaceholderImageName = "map-loading";

    static std::string makeImageName(uint32_t pMapId);

    RmlUiLayer& mRmlUiLayer;
    ImageReadyCallback mImageReadyCallback;
    MapGroundPreparer mPreparer;

    TerrainMaterial mMaterial;
    std::unique_ptr<Terrain> mTerrain;

    const MapData* mMap{nullptr};
    bool mAreLayersPending{true};
    bool mIsGeometryPending{false};
    bool mIsImagePending{false};

    bool updateLayers();
    bool updateGeometry();
    bool updateImage();
};

}

#endif
