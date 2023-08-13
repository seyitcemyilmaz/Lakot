#ifndef MATERIALLOADER_H
#define MATERIALLOADER_H

#include <assimp/scene.h>

#include "resource/ModelResource.h"

class MaterialLoader {
private:
    ModelResource* mModelResource;

	aiMaterial* mMaterial;

    const aiScene* mScene;

	std::string mModelPath;

	TextureResource* extractTexture(aiTextureType pAiTextureType, TextureType pTextureType);

public:
    MaterialLoader(ModelResource* pModelResource, aiMaterial* pMaterial, const aiScene* pScene, const std::string& pModelPath);

	MaterialResource* loadMaterial();
};

#endif
