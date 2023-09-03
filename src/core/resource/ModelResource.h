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
	std::vector<MaterialResource*> mMaterialResources;
	std::vector<TextureResource*> mTextureResources;
	std::vector<AnimationResource*> mAnimationResources;

	std::map<std::string, BoneResource*> mBoneMap;

	glm::mat4 mGlobalInverseTransform;

protected:
	void setRootNode(NodeResource* pRootNode);
	void setGlobalInverseTransform(const glm::mat4& pGlobalInverseTransform);

	void addMeshResource(MeshResource* pMeshResource);
	void addBoneResource(BoneResource* pBoneResource);
	void addMaterialResource(MaterialResource* pMaterialResource);
	void addAnimationResource(AnimationResource* pAnimationResource);

	unsigned int getBoneId(const std::string& pBoneName);
	friend class ModelLoader;
	friend class MaterialLoader;

public:
	ModelResource();

	NodeResource* getRootNodeResource();
	MaterialResource* getMaterialResource(int pMaterialIndex);

	const std::vector<TextureResource*>& getTextureResources();
	const std::vector<AnimationResource*>& getAnimationResources();
};

#endif
