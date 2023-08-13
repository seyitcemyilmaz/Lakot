#include "TextureResource.h"

TextureResource::TextureResource(unsigned int pTextureId, const std::string& pTexturePath) :
    mTextureId(pTextureId), mTexturePath(pTexturePath) { }

unsigned int TextureResource::getTextureId() const {
	return mTextureId;
}

const std::string& TextureResource::getTexturePath() const {
	return mTexturePath;
}
