#ifndef LAKOT_TEXTURERESOURCE_H
#define LAKOT_TEXTURERESOURCE_H

#include <string>

class TextureResource {
private:
	unsigned int mTextureId;

	std::string mTexturePath;

public:
	TextureResource(unsigned int pTextureId, std::string pTexturePath);

	std::string getTexturePath();
};


#endif
