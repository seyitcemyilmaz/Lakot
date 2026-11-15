#include "TerrainImageLoader.h"

#include "FileManager.h"

using namespace lakot;

namespace
{
    DecodedImage decodeLayer(const std::string& pDirectory, const char* pName)
    {
        std::string tError;
        return Texture::decodeFile(FileManager::createPath(pDirectory, pName), tError);
    }
}

TerrainLayerImages TerrainImageLoader::loadLayers(const std::string& pTextureDirectory)
{
    TerrainLayerImages tLayers;
    tLayers.grass = decodeLayer(pTextureDirectory, "grass.png");
    tLayers.dryGrass = decodeLayer(pTextureDirectory, "dry_grass.png");
    tLayers.dirt = decodeLayer(pTextureDirectory, "dirt.png");
    tLayers.road = decodeLayer(pTextureDirectory, "road.png");
    tLayers.rock = decodeLayer(pTextureDirectory, "rock.png");
    tLayers.macro = decodeLayer(pTextureDirectory, "macro.png");
    return tLayers;
}

DecodedImage TerrainImageLoader::loadSplat(const std::string& pMapDirectory)
{
    std::string tPath = FileManager::createPath(pMapDirectory, "splat.png");
    std::string tError;

    if (!FileManager::isFileExist(tPath))
    {
        return {};
    }

    return Texture::decodeFile(tPath, tError);
}
