#ifndef LAKOT_MESHLOADER_H
#define LAKOT_MESHLOADER_H

#include <assimp/scene.h>

#include "Loader.h"

namespace lakot {

class ModelAsset;
class NodeAsset;
class MeshAsset;

class MeshLoader : virtual public Loader
{
public:
    virtual ~MeshLoader() override;
    MeshLoader();

    void initialize() override;
    void deinitialize() override;

    IObject* load() override;

    void setSceneObject(const aiScene* pSceneObject);
    void setMeshObject(aiMesh* pMeshObject);

    void setModelAsset(ModelAsset* pModelAsset);
    void setNodeAsset(NodeAsset* pNodeAsset);

protected:
    const aiScene* mSceneObject;
    aiMesh* mMeshObject;

    ModelAsset* mModelAsset;
    NodeAsset* mNodeAsset;
};

}


#endif
