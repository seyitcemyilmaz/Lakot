#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>

#include "../../engine/GarbageCollector.h"

#include "LoaderFactory.h"
#include "NodeLoader.h"
#include "lakot/utilities/Timer.h"

using namespace lakot;

ModelLoader::~ModelLoader()
{

}

ModelLoader::ModelLoader()
    : Loader()
    , mModelAsset(nullptr)
    , mSceneObject(nullptr)
    , mImporter(nullptr)
{
    mType = LoaderType::eModel;
}

void ModelLoader::initialize()
{

}

void ModelLoader::deinitialize()
{
    mImporter->FreeScene();
    delete mImporter;
}

IObject* ModelLoader::load()
{
    unsigned int tModelImportFlags = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_FlipUVs | aiProcess_SplitByBoneCount;

    mImporter = new Assimp::Importer();
    mImporter->SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

    const aiScene* tSceneObject = mImporter->ReadFile(mPath, tModelImportFlags);

    if (!tSceneObject)
    {
        mImporter->FreeScene();
        delete tSceneObject;
        return nullptr;
    }

    // NodeAsset* tRootNode = loadNodeStructure(tSceneObject);

    // if (!tRootNode)
    // {
    //     spdlog::error("Node structure is not created.");
    //     tImporter.FreeScene();
    //     return nullptr;
    // }

    // mModelAsset = new ModelAsset();
    // mModelAsset->mRootNode = tRootNode;

    return mModelAsset;
}

void ModelLoader::setPath(const std::string& pPath)
{
    mPath = pPath;
}

NodeAsset* ModelLoader::loadNodeStructure(const aiScene* pSceneObject)
{
    Loader* tLoader = LoaderFactory::createLoader(LoaderType::eNode);

    if (tLoader->getType() != LoaderType::eNode)
    {
        spdlog::error("Loader (node) is not created.");
        return nullptr;
    }

    NodeLoader* tNodeLoader = dynamic_cast<NodeLoader*>(tLoader);

    if (!tNodeLoader)
    {
        spdlog::error("Node loader is not created.");
        return nullptr;
    }

    tNodeLoader->setSceneObject(pSceneObject);
    tNodeLoader->setModelAsset(mModelAsset);

    NodeAsset* tNodeAsset = dynamic_cast<NodeAsset*>(tNodeLoader->load());

    delete tLoader;
    tLoader = nullptr;

    return tNodeAsset;
}

void ModelLoader::loadMaterials(const aiScene* pScene)
{
    if (!pScene->HasMaterials())
    {
        return;
    }


}
