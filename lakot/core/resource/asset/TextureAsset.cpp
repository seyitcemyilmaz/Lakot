#include "TextureAsset.h"

using namespace lakot;

TextureAsset::~TextureAsset()
{
    if (mImage)
    {
        delete mImage;
        mImage = nullptr;
    }
}

TextureAsset::TextureAsset(const std::string& pPath, Image* pImage)
    : AAsset(AssetType::eTexture)
    , ATexture()
    , mPath(pPath)
    , mImage(pImage)
{

}

void TextureAsset::initialize()
{

}

void TextureAsset::deinitialize()
{

}

const std::string& TextureAsset::getPath() const
{
    return mPath;
}
