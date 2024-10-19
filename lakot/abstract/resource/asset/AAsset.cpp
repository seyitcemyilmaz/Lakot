#include "AAsset.h"

using namespace lakot;

AAsset::~AAsset()
{

}

AAsset::AAsset()
    : Object()
    , mType(AssetType::eNone)
{

}

const std::string& AAsset::getUUID() const noexcept
{
    return mUUID;
}

void AAsset::setUUID(const std::string& pUUID) noexcept
{
    mUUID = pUUID;
}

AssetType AAsset::getType() const noexcept
{
    return mType;
}
