#ifndef LAKOT_TERRAIN_IMAGE_LOADER_H
#define LAKOT_TERRAIN_IMAGE_LOADER_H

#include <string>

#include "TerrainLayerImages.h"

namespace lakot
{

class TerrainImageLoader
{
public:
    static TerrainLayerImages loadLayers(const std::string& pTextureDirectory);
    static DecodedImage loadSplat(const std::string& pMapDirectory);

private:
    TerrainImageLoader() = default;
    ~TerrainImageLoader() = default;
};

}

#endif
