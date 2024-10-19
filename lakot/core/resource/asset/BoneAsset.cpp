#include "BoneAsset.h"

using namespace lakot;

BoneAsset::~BoneAsset()
{

}

BoneAsset::BoneAsset(const std::string& pName)
    : mName(pName)
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
