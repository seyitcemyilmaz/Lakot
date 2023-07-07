#include "TextureResource.h"

TextureResource::TextureResource(unsigned int pTextureId, std::string pTexturePath) {
	mTextureId = pTextureId;
	mTexturePath = pTexturePath;
}

std::string TextureResource::getTexturePath() {
	return mTexturePath;
}
