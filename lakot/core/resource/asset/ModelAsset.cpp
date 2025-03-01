#include "ModelAsset.h"

#include "NodeAsset.h"
#include "MeshAsset.h"
#include "TextureAsset.h"

using namespace lakot;

ModelAsset::~ModelAsset()
{

}

ModelAsset::ModelAsset(const std::string& pPath)
    : AAsset(AssetType::eModel)
    , AModel()
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

const std::string& ModelAsset::getPath() const
{
    return mPath;
}

const std::vector<AMesh*> ModelAsset::getMeshes() const
{
    return mMeshes;
}

const std::vector<ANode*> ModelAsset::getNodes() const
{
    return mNodes;
}

const std::vector<ABone*> ModelAsset::getBones() const
{
    return mBones;
}

const std::vector<ATexture*> ModelAsset::getTextures() const
{
    return mTextures;
}

const std::vector<AMaterial*> ModelAsset::getMaterials() const
{
    return mMaterials;
}

ANode* ModelAsset::getRootNode() const
{
    return mRootNode;
}

void ModelAsset::addMesh(MeshAsset* pMesh)
{
    mMeshes.push_back(pMesh);
}

void ModelAsset::addTexture(TextureAsset* pTexture)
{
    mTextures.push_back(pTexture);
}
