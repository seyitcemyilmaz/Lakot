#ifndef LAKOT_MODELRESOURCE_H
#define LAKOT_MODELRESOURCE_H

#include <map>

#include "NodeResource.h"
#include "MeshResource.h"
#include "BoneResource.h"
#include "MaterialResource.h"

class ModelResource {
private:
	NodeResource* mRootNode = nullptr;

	std::vector<MeshResource*> mMeshResources;
	std::vector<BoneResource*> mBoneResources;
	std::vector<MaterialResource*> mMaterialResources;
    std::vector<TextureResource*> mTextureResources;

	std::map<std::string, BoneResource*> mBoneMap;

protected:
	void setRootNode(NodeResource* pRootNode);

	void addMeshResource(MeshResource* pMeshResource);
	void addBoneResource(BoneResource* pBoneResource);
	void addMaterialResource(MaterialResource* pMaterialResource);

	unsigned int getBoneId(const std::string& pBoneName);
	friend class ModelLoader;
    friend class MaterialLoader;

public:
	ModelResource();

	NodeResource* getRootNodeResource();
	MaterialResource* getMaterialResource(int pMaterialIndex);

    const std::vector<TextureResource*>& getTextureResources();
};

#endif
