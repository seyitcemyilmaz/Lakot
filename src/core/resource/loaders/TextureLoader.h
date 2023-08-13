#ifndef LAKOT_TEXTURELOADER_H
#define LAKOT_TEXTURELOADER_H

#include <iostream>
#include <vector>

#include <assimp/Importer.hpp>

#include "resource/TextureResource.h"

class TextureLoader {
private:
	std::string mTexturePath;
	std::string mModelPath;
	
	TextureType mTextureType;

    const aiScene* mScene;

	std::vector<std::string> mPossibleTextureLocations;

	void createPossibleTextureLocations();

	std::string getTexturePath();

	TextureResource* createEmbeddedTexture();
    TextureResource* createReferencedTexture();

    TextureResource* createTexture(unsigned char* pTextureData, int pWidth, int pHeight, int pChannelCount);

    unsigned int getColorMode(int tChannelCount);

public:
    TextureLoader(const aiScene* pScene, const std::string& pModelPath, TextureType pTextureType, const std::string& pTexturePath);

	TextureResource* loadTexture();
};

#endif
