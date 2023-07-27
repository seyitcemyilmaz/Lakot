#include "TextureResource.h"

TextureResource::TextureResource(unsigned int pTextureId, const std::string& pTexturePath) :
	mTextureId(pTextureId), mTexturePath(pTexturePath) { }

std::string TextureResource::getTexturePath() const {
	return mTexturePath;
}
