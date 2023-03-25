#ifndef LAKOT_MODELRESOURCE_H
#define LAKOT_MODELRESOURCE_H

#include <map>

#include "noderesource.h"
#include "meshresource.h"
#include "boneresource.h"

class ModelResource {
private:
    std::vector<MeshResource*> mMeshResources;

    NodeResource* mRootNode;

    std::vector<BoneResource*> mBones;

    std::map<std::string, BoneResource*> mBoneMap;

protected:
    void addMeshResource(MeshResource* pMeshResource);
    void addBoneResource(BoneResource* pBoneResource);
    void setRootNode(NodeResource* pRootNode);

    int getBoneId(std::string pBoneName);
    friend class ModelLoader;
public:
    ModelResource();
};

#endif
