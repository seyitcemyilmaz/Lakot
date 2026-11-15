#include "TerrainMaterial.h"

#include <utility>

#include <SDL3/SDL.h>
#include <glad/glad.h>

using namespace lakot;

bool TerrainMaterial::setLayers(const TerrainLayerImages& pLayers)
{
    if (!pLayers.isComplete())
    {
        SDL_Log("TerrainMaterial: ground textures missing, using vertex colours");
        return false;
    }

    mGrass = uploadLayer(pLayers.grass);
    mDryGrass = uploadLayer(pLayers.dryGrass);
    mDirt = uploadLayer(pLayers.dirt);
    mRoad = uploadLayer(pLayers.road);
    mRock = uploadLayer(pLayers.rock);
    mMacro = uploadLayer(pLayers.macro);
    return true;
}

void TerrainMaterial::setSplat(const DecodedImage& pSplat)
{
    mSplat = Texture::fromImage(pSplat, TextureFilterType::eTrilinear, TextureWrapType::eClamp);

    if (mSplat)
    {
        return;
    }

    DecodedImage tEmpty;
    tEmpty.width = 1;
    tEmpty.height = 1;
    tEmpty.pixels.assign(4, 0);
    mSplat = Texture::fromImage(tEmpty, TextureFilterType::eNearest, TextureWrapType::eClamp);
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

std::shared_ptr<Texture> TerrainMaterial::uploadLayer(const DecodedImage& pImage)
{
    return Texture::fromImage(pImage, TextureFilterType::eTrilinear, TextureWrapType::eRepeat);
}
