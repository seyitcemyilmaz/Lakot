#include "ModelResource.h"

ModelResource::ModelResource() {
	mRootNode = nullptr;
}

void ModelResource::addMeshResource(MeshResource* pMeshResource) {
	mMeshResources.push_back(pMeshResource);
}

void ModelResource::addBoneResource(BoneResource* pBoneResource) {
	mBoneResources.push_back(pBoneResource);
}

void ModelResource::addMaterialResource(MaterialResource* pMaterialResource) {
	mMaterialResources.push_back(pMaterialResource);
}

void ModelResource::setRootNode(NodeResource* pRootNode) {
	mRootNode = pRootNode;
}

int ModelResource::getBoneId(std::string pBoneName) {
	return std::distance(mBoneResources.begin(), std::find(mBoneResources.begin(), mBoneResources.end(), mBoneMap[pBoneName]));
}
