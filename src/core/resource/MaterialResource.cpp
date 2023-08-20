#include "MaterialResource.h"

MaterialResource::MaterialResource(
    const glm::vec3& pDiffuseColor,
    const glm::vec3& pSpecularColor,
    const glm::vec3& pEmissiveColor,
    const glm::vec3& pAmbientColor,

	float pShininess,
	float pOpacity) :

	mDiffuseColor(pDiffuseColor),
	mSpecularColor(pSpecularColor),
	mEmissiveColor(pEmissiveColor),
	mAmbientColor(pAmbientColor),

	mShininess(pShininess),
	mOpacity(pOpacity) { }


void MaterialResource::setDiffuseTexture(TextureResource* pTextureResource) {
    mDiffuseTexture = pTextureResource;
}

void MaterialResource::setSpecularTexture(TextureResource* pTextureResource) {
    mSpecularTexture = pTextureResource;
}

void MaterialResource::setAmbientTexture(TextureResource* pTextureResource) {
    mAmbientTexture = pTextureResource;
}

void MaterialResource::setEmissiveTexture(TextureResource* pTextureResource) {
    mEmissiveTexture = pTextureResource;
}

void MaterialResource::setHeightTexture(TextureResource* pTextureResource) {
    mHeightTexture = pTextureResource;
}

void MaterialResource::setNormalsTexture(TextureResource* pTextureResource) {
    mNormalsTexture = pTextureResource;
 }

void MaterialResource::setShininessTexture(TextureResource* pTextureResource) {
    mShininessTexture = pTextureResource;
}

void MaterialResource::setOpacityTexture(TextureResource* pTextureResource) {
    mOpacityTexture = pTextureResource;
}

void MaterialResource::setDisplacementTexture(TextureResource* pTextureResource) {
    mDisplacementTexture = pTextureResource;
}

void MaterialResource::setLightmapTexture(TextureResource* pTextureResource) {
    mLightmapTexture = pTextureResource;
}

void MaterialResource::setReflectionTexture(TextureResource* pTextureResource) {
    mReflectionTexture = pTextureResource;
}

void MaterialResource::setBaseColorTexture(TextureResource* pTextureResource) {
    mBaseColorTexture = pTextureResource;
}

void MaterialResource::setNormalCameraTexture(TextureResource* pTextureResource) {
    mNormalCameraTexture = pTextureResource;
}

void MaterialResource::setEmissionColorTexture(TextureResource* pTextureResource) {
    mEmissionColorTexture = pTextureResource;
}

void MaterialResource::setMetalnessTexture(TextureResource* pTextureResource) {
    mMetalnessTexture = pTextureResource;
}

void MaterialResource::setDiffuseRoughnessTexture(TextureResource* pTextureResource) {
    mDiffuseRoughnessTexture = pTextureResource;
}

void MaterialResource::setAmbientOcclusionTexture(TextureResource* pTextureResource) {
    mAmbientOcclusionTexture = pTextureResource;
}

void MaterialResource::setSheenTexture(TextureResource* pTextureResource) {
    mSheenTexture = pTextureResource;
}

void MaterialResource::setClearcoatTexture(TextureResource* pTextureResource) {
    mClearcoatTexture = pTextureResource;
}

void MaterialResource::setTransmissionTexture(TextureResource* pTextureResource) {
    mTransmissionTexture = pTextureResource;
}

TextureResource* MaterialResource::getDiffuseTexture() {
	return mDiffuseTexture;
}

TextureResource* MaterialResource::getSpecularTexture() {
    return mSpecularTexture;
}

TextureResource* MaterialResource::getAmbientTexture() {
    return mAmbientTexture;
}

TextureResource* MaterialResource::getEmissiveTexture() {
    return mEmissiveTexture;
}

TextureResource* MaterialResource::getHeightTexture() {
    return mHeightTexture;
}

TextureResource* MaterialResource::getNormalsTexture() {
    return mNormalsTexture;
}

TextureResource* MaterialResource::getShininessTexture() {
    return mShininessTexture;
}

TextureResource* MaterialResource::getOpacityTexture() {
    return mOpacityTexture;
}

TextureResource* MaterialResource::getDisplacementTexture() {
    return mDisplacementTexture;
}

TextureResource* MaterialResource::getLightmapTexture() {
    return mLightmapTexture;
}

TextureResource* MaterialResource::getReflectionTexture() {
    return mReflectionTexture;
}

TextureResource* MaterialResource::getBaseColorTexture() {
    return mBaseColorTexture;
}

TextureResource* MaterialResource::getNormalCameraTexture() {
    return mNormalCameraTexture;
}

TextureResource* MaterialResource::getEmissionColorTexture() {
    return mEmissionColorTexture;
}

TextureResource* MaterialResource::getMetalnessTexture() {
    return mMetalnessTexture;
}

TextureResource* MaterialResource::getDiffuseRoughnessTexture() {
    return mDiffuseRoughnessTexture;
}

TextureResource* MaterialResource::getAmbientOcclusionTexture() {
    return mAmbientOcclusionTexture;
}

TextureResource* MaterialResource::getSheenTexture() {
    return mSheenTexture;
}

TextureResource* MaterialResource::getClearcoatTexture() {
    return mClearcoatTexture;
}

TextureResource* MaterialResource::getTransmissionTexture() {
    return mTransmissionTexture;
}

const glm::vec3& MaterialResource::getDiffuseColor() {
	return mDiffuseColor;
}

const glm::vec3& MaterialResource::getSpecularColor() {
	return mSpecularColor;
}

const glm::vec3& MaterialResource::getEmissiveColor() {
	return mEmissiveColor;
}

const glm::vec3& MaterialResource::getAmbientColor() {
	return mAmbientColor;
}

float MaterialResource::getShininess() {
	return mShininess;
}

float MaterialResource::getOpacity() {
	return mOpacity;
}
