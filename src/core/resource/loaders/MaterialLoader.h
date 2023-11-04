#ifndef LAKOT_MATERIALLOADER_H
#define LAKOT_MATERIALLOADER_H

#include <assimp/scene.h>

#include "resource/ModelResource.h"

class MaterialLoader {
private:
	ModelResource* mModelResource;

	aiMaterial* mMaterial;

	const aiScene* mScene;

	std::string mModelPath;

	TextureResource* extractTexture(aiTextureType pAiTextureType);

public:
	MaterialLoader(ModelResource* pModelResource, aiMaterial* pMaterial, const aiScene* pScene, const std::string& pModelPath);

	MaterialResource* loadMaterial();
};

#endif
