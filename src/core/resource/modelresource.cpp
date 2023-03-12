#include "modelresource.h"

ModelResource::ModelResource() {
    mRootNode = nullptr;
}

void ModelResource::addMeshResource(MeshResource* pMeshResource) {
    mMeshResources.push_back(pMeshResource);
}

void ModelResource::addBoneResource(BoneResource* pBoneResource) {
    mBones.push_back(pBoneResource);
}

void ModelResource::setRootNode(NodeResource* pRootNode) {
    mRootNode = pRootNode;
}

int ModelResource::getBoneId(std::string pBoneName) {
    return std::distance(mBones.begin(), std::find(mBones.begin(), mBones.end(), mBoneMap[pBoneName]));
}
