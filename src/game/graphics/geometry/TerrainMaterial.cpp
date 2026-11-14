#include "TerrainMaterial.h"

#include <utility>

#include <SDL3/SDL.h>
#include <glad/glad.h>

#include "FileManager.h"

using namespace lakot;

bool TerrainMaterial::load(const std::string& pTextureDirectory, const std::string& pMapDirectory)
{
    mGrass = loadLayer(FileManager::createPath(pTextureDirectory, "grass.png"));
    mDryGrass = loadLayer(FileManager::createPath(pTextureDirectory, "dry_grass.png"));
    mDirt = loadLayer(FileManager::createPath(pTextureDirectory, "dirt.png"));
    mRoad = loadLayer(FileManager::createPath(pTextureDirectory, "road.png"));
    mRock = loadLayer(FileManager::createPath(pTextureDirectory, "rock.png"));
    mMacro = loadLayer(FileManager::createPath(pTextureDirectory, "macro.png"));

    if (!(mGrass && mDryGrass && mDirt && mRoad && mRock && mMacro))
    {
        SDL_Log("TerrainMaterial: ground textures missing in %s, using vertex colours", pTextureDirectory.c_str());
        return false;
    }

    mSplat = loadSplat(FileManager::createPath(pMapDirectory, "splat.png"));
    return isReady();
}

bool TerrainMaterial::isReady() const
{
    return mGrass && mDryGrass && mDirt && mRoad && mRock && mMacro && mSplat;
}

void TerrainMaterial::bind(ShaderProgram& pShader, float pWorldSize) const
{
    if (!isReady())
    {
        pShader.setInt("uUseDetail", 0);
        return;
    }

    const std::pair<const char*, const Texture*> tBindings[] = {
        { "uSplat", mSplat.get() },
        { "uGrass", mGrass.get() },
        { "uDryGrass", mDryGrass.get() },
        { "uDirt", mDirt.get() },
        { "uRoad", mRoad.get() },
        { "uRock", mRock.get() },
        { "uMacro", mMacro.get() },
    };

    unsigned int tUnit = kFirstUnit;

    for (const auto& tBinding : tBindings)
    {
        tBinding.second->bind(tUnit);
        pShader.setInt(tBinding.first, static_cast<int>(tUnit));
        ++tUnit;
    }

    glActiveTexture(GL_TEXTURE0);
    pShader.setInt("uUseDetail", 1);
    pShader.setFloat("uWorldSize", pWorldSize);
}

std::shared_ptr<Texture> TerrainMaterial::loadLayer(const std::string& pPath)
{
    std::string tError;
    return Texture::fromFile(pPath, tError, TextureFilterType::eTrilinear, TextureWrapType::eRepeat);
}

std::shared_ptr<Texture> TerrainMaterial::loadSplat(const std::string& pPath)
{
    std::string tError;
    std::shared_ptr<Texture> tSplat;

    if (FileManager::isFileExist(pPath))
    {
        tSplat = Texture::fromFile(pPath, tError, TextureFilterType::eTrilinear, TextureWrapType::eClamp);
    }

    if (tSplat)
    {
        return tSplat;
    }

    DecodedImage tEmpty;
    tEmpty.width = 1;
    tEmpty.height = 1;
    tEmpty.pixels.assign(4, 0);
    return Texture::fromImage(tEmpty, TextureFilterType::eNearest, TextureWrapType::eClamp);
}
