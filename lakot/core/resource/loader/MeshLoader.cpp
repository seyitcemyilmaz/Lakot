#include "MeshLoader.h"

using namespace lakot;

MeshLoader::~MeshLoader()
{

}

MeshLoader::MeshLoader()
    : Loader()
    , mSceneObject(nullptr)
    , mMeshObject(nullptr)
    , mModelAsset(nullptr)
    , mNodeAsset(nullptr)
{
    mType = LoaderType::eMesh;
}

void MeshLoader::initialize()
{

}

void MeshLoader::deinitialize()
{

}

IObject* MeshLoader::load()
{
    if (!mSceneObject)
    {
        spdlog::error("Scene object is not found.");
        return nullptr;
    }

    if (!mMeshObject)
    {
        spdlog::error("Mesh object is not found.");
        return nullptr;
    }

    if (!mModelAsset)
    {
        spdlog::error("Model asset is not found.");
        return nullptr;
    }

    if (!mNodeAsset)
    {
        spdlog::error("Node asset is not found.");
        return nullptr;
    }


}

void MeshLoader::setSceneObject(const aiScene* pSceneObject)
{
    mSceneObject = pSceneObject;
}

void MeshLoader::setMeshObject(aiMesh* pMeshObject)
{
    mMeshObject = pMeshObject;
}

void MeshLoader::setModelAsset(ModelAsset* pModelAsset)
{
    mModelAsset = pModelAsset;
}

void MeshLoader::setNodeAsset(NodeAsset* pNodeAsset)
{
    mNodeAsset = pNodeAsset;
}
