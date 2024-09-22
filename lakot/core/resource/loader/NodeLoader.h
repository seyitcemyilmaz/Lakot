#ifndef LAKOT_NODELOADER_H
#define LAKOT_NODELOADER_H

#include <assimp/scene.h>

#include "Loader.h"

namespace lakot {

class ModelAsset;
class NodeAsset;

class NodeLoader : virtual public Loader
{
public:
    virtual ~NodeLoader() override;
    NodeLoader();

    void initialize() override;
    void deinitialize() override;

    IObject* load() override;

    void setSceneObject(const aiScene* pSceneObject);
    void setModelAsset(ModelAsset* pModelAsset);

protected:
    const aiScene* mSceneObject;

    ModelAsset* mModelAsset;

    NodeAsset* loadNode(aiNode* pNode, NodeAsset* pParentNodeAsset);
};

}

#endif
