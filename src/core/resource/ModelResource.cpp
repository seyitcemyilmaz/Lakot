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

unsigned int ModelResource::getBoneId(const std::string& pBoneName) {
	return static_cast<unsigned int>(std::distance(mBoneResources.begin(), std::ranges::find(mBoneResources, mBoneMap[pBoneName])));
}

NodeResource* ModelResource::getRootNodeResource() {
	return mRootNode;
}
