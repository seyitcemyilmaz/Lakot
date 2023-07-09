#include "MaterialLoader.h"

#include "TextureLoader.h"

MaterialLoader::MaterialLoader(aiMaterial* pMaterial, std::string pModelPath) {
	mMaterial = pMaterial;
	mModelPath = pModelPath;
}

MaterialResource* MaterialLoader::loadMaterial() {
	MaterialResource* tMaterialResource = new MaterialResource();

	TextureResource* tDiffuseTexture = loadDiffuseTexture();
	TextureResource* tNormalTexture = loadNormalTexture();
	TextureResource* tSpecularTexture = loadSpecularTexture();
	TextureResource* tEmmisiveTexture = loadEmmisiveTexture();
	TextureResource* tAmbientTexture = loadAmbientTexture();

	if (tDiffuseTexture) {
		tMaterialResource->setDiffuseTexture(tDiffuseTexture);
	}

	if (tNormalTexture) {
		tMaterialResource->setNormalTexture(tNormalTexture);
	}

	if (tSpecularTexture) {
		tMaterialResource->setSpecularTexture(tSpecularTexture);
	}

	if (tEmmisiveTexture) {
		tMaterialResource->setEmissiveTexture(tEmmisiveTexture);
	}

	if (tAmbientTexture) {
		tMaterialResource->setAmbientTexture(tAmbientTexture);
	}

	return tMaterialResource;
}

TextureResource* MaterialLoader::loadDiffuseTexture() {
	unsigned int tDiffuseTextureCount = mMaterial->GetTextureCount(aiTextureType_DIFFUSE);

	if (tDiffuseTextureCount == 0) {
		return nullptr;
	}

	if (tDiffuseTextureCount > 1) {
		throw "Diffuse texture count is bigger than 1";
	}

	return extractTexture(aiTextureType_DIFFUSE);
}

TextureResource* MaterialLoader::loadNormalTexture() {
	unsigned int tNormalTextureCount = mMaterial->GetTextureCount(aiTextureType_NORMALS);

	if (tNormalTextureCount == 0) {
		return nullptr;
	}

	if (tNormalTextureCount > 1) {
		throw "Normal texture count is bigger than 1";
	}

	return extractTexture(aiTextureType_NORMALS);
}

TextureResource* MaterialLoader::loadSpecularTexture() {
	unsigned int tSpecularTextureCount = mMaterial->GetTextureCount(aiTextureType_SPECULAR);

	if (tSpecularTextureCount == 0) {
		return nullptr;
	}

	if (tSpecularTextureCount > 1) {
		throw "Specular Texture count is bigger than 1";
	}

	return extractTexture(aiTextureType_SPECULAR);
}

TextureResource* MaterialLoader::loadEmmisiveTexture() {
	unsigned int tEmissiveTextureCount = mMaterial->GetTextureCount(aiTextureType_EMISSIVE);

	if (tEmissiveTextureCount == 0) {
		return nullptr;
	}

	if (tEmissiveTextureCount > 1) {
		throw "Emissive texture count is bigger than 1";
	}

	return extractTexture(aiTextureType_EMISSIVE);
}

TextureResource* MaterialLoader::loadAmbientTexture() {
	unsigned int tAmbientTextureCount = mMaterial->GetTextureCount(aiTextureType_AMBIENT);

	if (tAmbientTextureCount == 0) {
		return nullptr;
	}

	if (tAmbientTextureCount > 1) {
		throw "Ambient texture count is bigger than 1";
	}

	return extractTexture(aiTextureType_AMBIENT);
}

TextureResource* MaterialLoader::extractTexture(aiTextureType pTextureType) {
	aiString tTempFilePath;

	if (mMaterial->Get(AI_MATKEY_TEXTURE(pTextureType, 0), tTempFilePath) != AI_SUCCESS) {
		throw "Texture is not found.";
	}

	if (tTempFilePath.length == 0) {
		throw "Texture file path cannot be empty.";
	}

	std::string tTexturePath = tTempFilePath.C_Str();

	TextureLoader tTextureLoader(mModelPath, tTexturePath);

	return tTextureLoader.loadTexture();
}
