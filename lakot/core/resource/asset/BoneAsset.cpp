#include "BoneAsset.h"

using namespace lakot;

BoneAsset::~BoneAsset()
{

}

BoneAsset::BoneAsset(const std::string& pName, const glm::mat4& pTransformation)
    : AAsset(AssetType::eBone)
    , ABone(pTransformation)
    , mName(pName)
{

}

void BoneAsset::initialize()
{

}

void BoneAsset::deinitialize()
{

}

const std::string& BoneAsset::getName() const
{
    return mName;
}
