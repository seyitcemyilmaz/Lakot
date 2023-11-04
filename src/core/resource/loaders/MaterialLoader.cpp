#include "MaterialLoader.h"

#include <glm/glm.hpp>

#include "TextureLoader.h"

MaterialLoader::MaterialLoader(ModelResource* pModelResource, aiMaterial* pMaterial, const aiScene* pScene, const std::string& pModelPath)
	: mModelResource(pModelResource)
	, mMaterial(pMaterial)
	, mScene(pScene)
	, mModelPath(pModelPath) { }

MaterialResource* MaterialLoader::loadMaterial() {
	glm::vec3 tDiffuseColor(0.0f);
	glm::vec3 tSpecularColor(0.0f);
	glm::vec3 tEmissiveColor(0.0f);
	glm::vec3 tAmbientColor(0.0f);

	float tShininess = 1.0f;
	float tOpacity = 1.0f;

	aiColor3D tTempColor;
	float tTempValue;

	if (mMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, tTempColor) == AI_SUCCESS) {
		tDiffuseColor = glm::vec3(tTempColor.r, tTempColor.g, tTempColor.b);
	}

	if (mMaterial->Get(AI_MATKEY_COLOR_SPECULAR, tTempColor) == AI_SUCCESS) {
		tSpecularColor = glm::vec3(tTempColor.r, tTempColor.g, tTempColor.b);
	}

	if (mMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, tTempColor) == AI_SUCCESS) {
		tEmissiveColor = glm::vec3(tTempColor.r, tTempColor.g, tTempColor.b);
	}

	if (mMaterial->Get(AI_MATKEY_COLOR_AMBIENT, tTempColor) == AI_SUCCESS) {
		tAmbientColor = glm::vec3(tTempColor.r, tTempColor.g, tTempColor.b);
	}

	if (mMaterial->Get(AI_MATKEY_SHININESS, tTempValue) == AI_SUCCESS) {
		tShininess = tTempValue;
	}

	if (mMaterial->Get(AI_MATKEY_OPACITY, tTempValue) == AI_SUCCESS) {
		tOpacity = tTempValue;
	}

	MaterialResource* tMaterialResource = new MaterialResource(tDiffuseColor, tSpecularColor, tEmissiveColor, tAmbientColor, tShininess, tOpacity);

	TextureResource* tDiffuseTexture = extractTexture(aiTextureType_DIFFUSE);
	tMaterialResource->setDiffuseTexture(tDiffuseTexture);

	TextureResource* tSpecularTexture = extractTexture(aiTextureType_SPECULAR);
	tMaterialResource->setSpecularTexture(tSpecularTexture);

	TextureResource* tAmbientTexture = extractTexture(aiTextureType_AMBIENT);
	tMaterialResource->setAmbientTexture(tAmbientTexture);

	TextureResource* tEmissiveTexture = extractTexture(aiTextureType_EMISSIVE);
	tMaterialResource->setEmissiveTexture(tEmissiveTexture);

	TextureResource* tHeightTexture = extractTexture(aiTextureType_HEIGHT);
	tMaterialResource->setHeightTexture(tHeightTexture);

	TextureResource* tNormalsTexture = extractTexture(aiTextureType_NORMALS);
	tMaterialResource->setNormalsTexture(tNormalsTexture);

	TextureResource* tShininessTexture = extractTexture(aiTextureType_SHININESS);
	tMaterialResource->setShininessTexture(tShininessTexture);

	TextureResource* tOpacityTexture = extractTexture(aiTextureType_OPACITY);
	tMaterialResource->setOpacityTexture(tOpacityTexture);

	TextureResource* tDisplacementTexture = extractTexture(aiTextureType_DISPLACEMENT);
	tMaterialResource->setDisplacementTexture(tDisplacementTexture);

	TextureResource* tLightmapTexture = extractTexture(aiTextureType_LIGHTMAP);
	tMaterialResource->setLightmapTexture(tLightmapTexture);

	TextureResource* tReflectionTexture = extractTexture(aiTextureType_REFLECTION);
	tMaterialResource->setReflectionTexture(tReflectionTexture);

	TextureResource* tBaseColorTexture = extractTexture(aiTextureType_BASE_COLOR);
	tMaterialResource->setBaseColorTexture(tBaseColorTexture);

	TextureResource* tNormalCameraTexture = extractTexture(aiTextureType_NORMAL_CAMERA);
	tMaterialResource->setNormalCameraTexture(tNormalCameraTexture);

	TextureResource* tEmissionColorTexture = extractTexture(aiTextureType_EMISSION_COLOR);
	tMaterialResource->setEmissionColorTexture(tEmissionColorTexture);

	TextureResource* tMetalnessTexture = extractTexture(aiTextureType_METALNESS);
	tMaterialResource->setMetalnessTexture(tMetalnessTexture);

	TextureResource* tDiffuseRoughnessTexture = extractTexture(aiTextureType_DIFFUSE_ROUGHNESS);
	tMaterialResource->setDiffuseRoughnessTexture(tDiffuseRoughnessTexture);

	TextureResource* tAmbientOcclusionTexture = extractTexture(aiTextureType_AMBIENT_OCCLUSION);
	tMaterialResource->setAmbientOcclusionTexture(tAmbientOcclusionTexture);

	TextureResource* tSheenTexture = extractTexture(aiTextureType_SHEEN);
	tMaterialResource->setSheenTexture(tSheenTexture);

	TextureResource* tClearcoatTexture = extractTexture(aiTextureType_CLEARCOAT);
	tMaterialResource->setClearcoatTexture(tClearcoatTexture);

	TextureResource* tTransmissionTexture = extractTexture(aiTextureType_TRANSMISSION);
	tMaterialResource->setTransmissionTexture(tTransmissionTexture);

	return tMaterialResource;
}

TextureResource* MaterialLoader::extractTexture(aiTextureType pAiTextureType) {
	unsigned int tTextureCount = mMaterial->GetTextureCount(pAiTextureType);

	if (tTextureCount == 0) {
		return nullptr;
	}

	if (tTextureCount > 1) {
		throw "Texture count is bigger than 1";
	}

	aiString tTempFilePath;

	if (mMaterial->Get(AI_MATKEY_TEXTURE(pAiTextureType, 0), tTempFilePath) != AI_SUCCESS) {
		throw "Texture is not found.";
	}

	if (tTempFilePath.length == 0) {
		throw "Texture file path cannot be empty.";
	}

	std::string tTexturePath = tTempFilePath.C_Str();

	TextureResource* tTexture = nullptr;

	const std::vector<TextureResource*>& tTextureResources = mModelResource->getTextureResources();

	size_t tTextureResourceCount = tTextureResources.size();

	for (size_t i = 0; i < tTextureResourceCount; i++) {
		TextureResource* tTextureResource = tTextureResources[i];
		const std::string& tTextureResourceName = tTextureResource->getTexturePath();

		if (tTexturePath == tTextureResourceName) {
			tTexture = tTextureResource;
			break;
		}
	}

	if (!tTexture) {
		TextureLoader tTextureLoader(mScene, mModelPath, tTexturePath);
		tTexture = tTextureLoader.loadTexture();

		if (tTexture) {
			mModelResource->mTextureResources.push_back(tTexture);
		}
	}

	return tTexture;
}
