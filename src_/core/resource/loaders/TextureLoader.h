#ifndef LAKOT_TEXTURELOADER_H
#define LAKOT_TEXTURELOADER_H

#include <iostream>
#include <vector>

#include "resource/TextureResource.h"

class TextureLoader {
private:
	std::string mTexturePath;
	std::string mModelPath;

	std::vector<std::string> mPossibleTextureLocations;

	void createPossibleTextureLocations();

	std::string getTexturePath();

	TextureResource* createEmbeddedTexture();
	TextureResource* createReferencedTexture();
public:
	TextureLoader(std::string pModelPath, std::string pTexturePath);

	TextureResource* loadTexture();
};

#endif
