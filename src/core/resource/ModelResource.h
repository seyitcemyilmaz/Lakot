#ifndef LAKOT_MODELRESOURCE_H
#define LAKOT_MODELRESOURCE_H

#include <map>

#include "AnimationResource.h"
#include "BoneResource.h"
#include "MaterialResource.h"
#include "MeshResource.h"
#include "NodeResource.h"

class ModelResource {
private:
	NodeResource* mRootNode = nullptr;

	std::vector<MeshResource*> mMeshResources;
	std::vector<BoneResource*> mBoneResources;
	std::vector<NodeResource*> mNodeResources;
	std::vector<MaterialResource*> mMaterialResources;
	std::vector<TextureResource*> mTextureResources;
	std::vector<AnimationResource*> mAnimationResources;

	glm::mat4 mGlobalInverseTransform;

protected:
	void setRootNode(NodeResource* pRootNode);
	void setGlobalInverseTransform(const glm::mat4& pGlobalInverseTransform);

	void addMeshResource(MeshResource* pMeshResource);
	void addBoneResource(BoneResource* pBoneResource);

	friend class ModelLoader;
	friend class MaterialLoader;

public:
	ModelResource();

	unsigned int getMeshCount();
	unsigned int getBoneCount();
	unsigned int getNodeCount();
	unsigned int getMaterialCount();
	unsigned int getAnimationCount();

	const glm::mat4& getGlobalInverseTransform();

	NodeResource* getRootNodeResource();
	MaterialResource* getMaterialResource(int pMaterialIndex);

	unsigned int getBoneId(BoneResource* pBoneResource);

	const std::vector<BoneResource*>& getBoneResources();
	const std::vector<TextureResource*>& getTextureResources();
	const std::vector<MaterialResource*>& getMaterialResources();
	const std::vector<AnimationResource*>& getAnimationResources();
};

#endif
