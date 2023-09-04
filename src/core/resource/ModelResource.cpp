#include "ModelResource.h"

ModelResource::ModelResource() = default;

unsigned int ModelResource::getMeshCount() {
	return static_cast<unsigned int>(mMeshResources.size());
}

unsigned int ModelResource::getNodeCount() {
	return static_cast<unsigned int>(mNodeResources.size());
}

unsigned int ModelResource::getAnimationCount() {
	return static_cast<unsigned int>(mAnimationResources.size());
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

void ModelResource::addAnimationResource(AnimationResource* pAnimationResource) {
	mAnimationResources.push_back(pAnimationResource);
}

void ModelResource::setRootNode(NodeResource* pRootNode) {
	mRootNode = pRootNode;
}

void ModelResource::setGlobalInverseTransform(const glm::mat4& pGlobalInverseTransform) {
	mGlobalInverseTransform = pGlobalInverseTransform;
}

unsigned int ModelResource::getBoneId(BoneResource* pBoneResource) {
	return static_cast<unsigned int>(std::distance(mBoneResources.begin(), std::ranges::find(mBoneResources, pBoneResource)));
}

NodeResource* ModelResource::getRootNodeResource() {
	return mRootNode;
}

MaterialResource* ModelResource::getMaterialResource(int pMaterialIndex) {
	if (pMaterialIndex >= mMaterialResources.size() || pMaterialIndex < 0) {
		throw "Wrong material index.";
	}

	return mMaterialResources[pMaterialIndex];
}

const std::vector<BoneResource*>& ModelResource::getBoneResources() {
	return mBoneResources;
}

const std::vector<TextureResource*>& ModelResource::getTextureResources() {
	return mTextureResources;
}

const std::vector<AnimationResource*>& ModelResource::getAnimationResources() {
	return mAnimationResources;
}
