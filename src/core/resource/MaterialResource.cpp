#include "MaterialResource.h"

MaterialResource::MaterialResource() = default;

void MaterialResource::setDiffuseTexture(TextureResource* pDiffuseTexture) {
	mDiffuseTexture = pDiffuseTexture;
}

void MaterialResource::setNormalTexture(TextureResource* pNormalTexture) {
	mNormalTexture = pNormalTexture;
}

void MaterialResource::setSpecularTexture(TextureResource* pSpecularTexture) {
	mSpecularTexture = pSpecularTexture;
}

void MaterialResource::setEmissiveTexture(TextureResource* pEmmisiveTexture) {
	mEmmisiveTexture = pEmmisiveTexture;
}

void MaterialResource::setAmbientTexture(TextureResource* pAmbientTexture) {
	mAmbientTexture = pAmbientTexture;
}