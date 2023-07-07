#ifndef MATERIALLOADER_H
#define MATERIALLOADER_H

#include <assimp/scene.h>

#include "resource/MaterialResource.h"
#include "resource/textureresource.h"

class MaterialLoader
{
private:
	aiMaterial* mMaterial;

	std::string mModelPath;

	TextureResource* loadDiffuseTexture();
	TextureResource* loadNormalTexture();
	TextureResource* loadSpecularTexture();
	TextureResource* loadEmmisiveTexture();
	TextureResource* loadAmbientTexture();

	TextureResource* extractTexture(aiTextureType pTextureType);

public:
	MaterialLoader(aiMaterial* pMaterial, std::string pModelPath);

	MaterialResource* loadMaterial();
};

#endif
