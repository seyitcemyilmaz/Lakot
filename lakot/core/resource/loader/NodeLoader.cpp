#include "NodeLoader.h"

// #include "../asset/ModelAsset.h"
#include "../asset/NodeAsset.h"
#include "../asset/MeshAsset.h"

#include "LoaderFactory.h"
#include "MeshLoader.h"

using namespace lakot;

NodeLoader::~NodeLoader()
{
    mSceneObject = nullptr;
}

NodeLoader::NodeLoader()
    : Loader()
    , mSceneObject(nullptr)
    , mModelAsset(nullptr)
{
    mType = LoaderType::eNode;
}

void NodeLoader::initialize()
{

}

void NodeLoader::deinitialize()
{

}

IObject* NodeLoader::load()
{
    if (!mSceneObject)
    {
        spdlog::error("Scene object is not found.");
        return nullptr;
    }

    if (!mModelAsset)
    {
        spdlog::error("Model asset is not found.");
        return nullptr;
    }

    aiNode* tRootNode = mSceneObject->mRootNode;

    if (!tRootNode)
    {
        spdlog::error("Root node is not found.");
        return nullptr;
    }

    NodeAsset* tRootNodeAsset = loadNode(tRootNode, nullptr);

    if (!tRootNodeAsset)
    {
        spdlog::error("Root node is not created.");
        return nullptr;
    }

    return tRootNodeAsset;
}

void NodeLoader::setSceneObject(const aiScene* pSceneObject)
{
    mSceneObject = pSceneObject;
}

void NodeLoader::setModelAsset(ModelAsset* pModelAsset)
{
    mModelAsset = pModelAsset;
}

NodeAsset* NodeLoader::loadNode(aiNode* pNode, NodeAsset* pParentNodeAsset)
{
    NodeAsset* tReturnValue = nullptr;

    std::string pNodeName = pNode->mName.C_Str();

    aiMatrix4x4& tMatrix = pNode->mTransformation;

    spdlog::info(pNode->mMetaData->mNumProperties);

    glm::mat4 tTransform
    {
        tMatrix.a1, tMatrix.b1, tMatrix.c1, tMatrix.d1,
        tMatrix.a2, tMatrix.b2, tMatrix.c2, tMatrix.d2,
        tMatrix.a3, tMatrix.b3, tMatrix.c3, tMatrix.d3,
        tMatrix.a4, tMatrix.b4, tMatrix.c4, tMatrix.d4
    };

    NodeAsset* tNode = new NodeAsset(pNodeName, tTransform, pParentNodeAsset);

    if (!pParentNodeAsset)
    {
        tReturnValue = tNode;
    }
    else
    {
        pParentNodeAsset->addChildNode(tNode);
    }

    unsigned int tMeshCount = mSceneObject->mNumMeshes;

    for (unsigned int i = 0; i < tMeshCount; i++)
    {
        unsigned int tMeshIndice = pNode->mMeshes[i];
        aiMesh* tMeshObject = mSceneObject->mMeshes[tMeshIndice];

        if (!tMeshObject)
        {
            spdlog::error("Mesh object is not found.");
            continue;
        }

        Loader* tLoader = LoaderFactory::createLoader(LoaderType::eMesh);

        if (!tLoader)
        {
            spdlog::error("Mesh loader is not created.");
            continue;
        }

        MeshLoader* tMeshLoader = dynamic_cast<MeshLoader*>(tLoader);

        if (!tMeshLoader)
        {
            spdlog::error("Return type should be MeshLoader*.");
            // TODO: add delete tLoader;
            continue;
        }

        tMeshLoader->setSceneObject(mSceneObject);
        tMeshLoader->setMeshObject(tMeshObject);

        tMeshLoader->setModelAsset(mModelAsset);
        tMeshLoader->setNodeAsset(tNode);

        IObject* tObject = tMeshLoader->load();

        if (!tObject)
        {
            spdlog::error("Mesh asset is not created.");
            // TODO: add delete tLoader;
            continue;
        }

        MeshAsset* tMeshAsset = dynamic_cast<MeshAsset*>(tObject);

        if (!tMeshAsset)
        {
            spdlog::error("Return type should be MeshAsset*.");
            // TODO: add delete tLoader;
            // TODO: add delete tObject;
            continue;
        }

        tNode->addChildMesh(tMeshAsset);
    }

    unsigned int tChildNodeCount = pNode->mNumChildren;

    for (unsigned int i = 0; i < tChildNodeCount; i++)
    {
        loadNode(pNode->mChildren[i], tNode);
    }

    return tReturnValue;
}
