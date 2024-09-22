#ifndef LAKOT_MODELLOADER_H
#define LAKOT_MODELLOADER_H

#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "Loader.h"

#include "../asset/ModelAsset.h"
#include "../asset/NodeAsset.h"

namespace lakot {

class ModelLoader : virtual public Loader
{
public:
    virtual ~ModelLoader() override;
    ModelLoader();

    void initialize() override;
    void deinitialize() override;

    IObject* load() override;

    void setPath(const std::string& pPath);

private:
    std::string mPath;

    ModelAsset* mModelAsset;

    Assimp::Importer* mImporter;
    const aiScene* mSceneObject;

    NodeAsset* loadNodeStructure(const aiScene* pSceneObject);

    void loadMaterials(const aiScene* pSceneObject);
};

}

#endif
