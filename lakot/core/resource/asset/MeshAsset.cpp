#include "MeshAsset.h"

using namespace lakot;

MeshAsset::~MeshAsset()
{

}

MeshAsset::MeshAsset(const std::string& pName, unsigned int pMaterialIndex)
    : AAsset()
    , mName(pName)
    , mBoundingBox(nullptr)
    , mVertexInformation(nullptr)
    , mMaterialIndex(pMaterialIndex)
    , mHasBones(false)
{

}

void MeshAsset::initialize()
{

}

void MeshAsset::deinitialize()
{

}

const std::string& MeshAsset::getName() const
{
    return mName;
}

BoundingBox* MeshAsset::getBoundingBox() const
{
    return mBoundingBox;
}

VertexInformation* MeshAsset::getVertexInformation() const
{
    return mVertexInformation;
}

unsigned int MeshAsset::getMaterialIndex() const
{
    return mMaterialIndex;
}

bool MeshAsset::getHasBones() const
{
    return mHasBones;
}

void MeshAsset::setBoundingBox(BoundingBox* pBoundingBox)
{
    mBoundingBox = pBoundingBox;
}

void MeshAsset::setVertexInformation(VertexInformation* pVertexInformation)
{
    mVertexInformation = pVertexInformation;
}

void MeshAsset::setHasBones(bool pHasBones)
{
    mHasBones = pHasBones;
}
