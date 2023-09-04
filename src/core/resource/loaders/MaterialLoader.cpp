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

	TextureResource* tDiffuseTexture = extractTexture(aiTextureType_DIFFUSE, TextureType::eDiffuseTexture);
	tMaterialResource->setDiffuseTexture(tDiffuseTexture);

	TextureResource* tSpecularTexture = extractTexture(aiTextureType_SPECULAR, TextureType::eSpecularTexture);
	tMaterialResource->setSpecularTexture(tSpecularTexture);

	TextureResource* tAmbientTexture = extractTexture(aiTextureType_AMBIENT, TextureType::eAmbientTexture);
	tMaterialResource->setAmbientTexture(tAmbientTexture);

	TextureResource* tEmissiveTexture = extractTexture(aiTextureType_EMISSIVE, TextureType::eEmissiveTexture);
	tMaterialResource->setEmissiveTexture(tEmissiveTexture);

	TextureResource* tHeightTexture = extractTexture(aiTextureType_HEIGHT, TextureType::eHeightTexture);
	tMaterialResource->setHeightTexture(tHeightTexture);

	TextureResource* tNormalsTexture = extractTexture(aiTextureType_NORMALS, TextureType::eNormalsTexture);
	tMaterialResource->setNormalsTexture(tNormalsTexture);

	TextureResource* tShininessTexture = extractTexture(aiTextureType_SHININESS, TextureType::eShininessTexture);
	tMaterialResource->setShininessTexture(tShininessTexture);

	TextureResource* tOpacityTexture = extractTexture(aiTextureType_OPACITY, TextureType::eOpacityTexture);
	tMaterialResource->setOpacityTexture(tOpacityTexture);

	TextureResource* tDisplacementTexture = extractTexture(aiTextureType_DISPLACEMENT, TextureType::eDisplacementTexture);
	tMaterialResource->setDisplacementTexture(tDisplacementTexture);

	TextureResource* tLightmapTexture = extractTexture(aiTextureType_LIGHTMAP, TextureType::eLightmapTexture);
	tMaterialResource->setLightmapTexture(tLightmapTexture);

	TextureResource* tReflectionTexture = extractTexture(aiTextureType_REFLECTION, TextureType::eReflectionTexture);
	tMaterialResource->setReflectionTexture(tReflectionTexture);

	TextureResource* tBaseColorTexture = extractTexture(aiTextureType_BASE_COLOR, TextureType::eBaseColorTexture);
	tMaterialResource->setBaseColorTexture(tBaseColorTexture);

	TextureResource* tNormalCameraTexture = extractTexture(aiTextureType_NORMAL_CAMERA, TextureType::eNormalCameraTexture);
	tMaterialResource->setNormalCameraTexture(tNormalCameraTexture);

	TextureResource* tEmissionColorTexture = extractTexture(aiTextureType_EMISSION_COLOR, TextureType::eEmissionColorTexture);
	tMaterialResource->setEmissionColorTexture(tEmissionColorTexture);

	TextureResource* tMetalnessTexture = extractTexture(aiTextureType_METALNESS, TextureType::eMetalnessTexture);
	tMaterialResource->setMetalnessTexture(tMetalnessTexture);

	TextureResource* tDiffuseRoughnessTexture = extractTexture(aiTextureType_DIFFUSE_ROUGHNESS, TextureType::eDiffuseRoughnessTexture);
	tMaterialResource->setDiffuseRoughnessTexture(tDiffuseRoughnessTexture);

	TextureResource* tAmbientOcclusionTexture = extractTexture(aiTextureType_AMBIENT_OCCLUSION, TextureType::eAmbientOcclusionTexture);
	tMaterialResource->setAmbientOcclusionTexture(tAmbientOcclusionTexture);

	TextureResource* tSheenTexture = extractTexture(aiTextureType_SHEEN, TextureType::eSheenTexture);
	tMaterialResource->setSheenTexture(tSheenTexture);

	TextureResource* tClearcoatTexture = extractTexture(aiTextureType_CLEARCOAT, TextureType::eClearcoatTexture);
	tMaterialResource->setClearcoatTexture(tClearcoatTexture);

	TextureResource* tTransmissionTexture = extractTexture(aiTextureType_TRANSMISSION, TextureType::eTransmissionTexture);
	tMaterialResource->setTransmissionTexture(tTransmissionTexture);

	return tMaterialResource;
}

TextureResource* MaterialLoader::extractTexture(aiTextureType pAiTextureType, TextureType pTextureType) {
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
		TextureLoader tTextureLoader(mScene, mModelPath, pTextureType, tTexturePath);
		tTexture = tTextureLoader.loadTexture();
		mModelResource->mTextureResources.push_back(tTexture);
	}

	return tTexture;
}
