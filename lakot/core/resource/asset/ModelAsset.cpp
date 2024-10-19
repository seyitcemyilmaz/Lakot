#include "ModelAsset.h"

using namespace lakot;

ModelAsset::~ModelAsset()
{

}

ModelAsset::ModelAsset(const std::string& pPath)
    : AAsset()
    , mPath(pPath)
{

}

void ModelAsset::initialize()
{
    // Bind to graphics api
}

void ModelAsset::deinitialize()
{
    // deinit
}

NodeAsset* ModelAsset::getRootNode() const
{
    return mRootNode;
}

const std::vector<TextureAsset*>& ModelAsset::getTextures() const
{
    return mTextures;
}

void ModelAsset::addMesh(MeshAsset* pMesh)
{
    mMeshes.push_back(pMesh);
}

void ModelAsset::addTexture(TextureAsset* pTexture)
{
    mTextures.push_back(pTexture);
}
