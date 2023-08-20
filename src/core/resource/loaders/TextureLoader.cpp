#include "TextureLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "helper/FileManager.h"

TextureLoader::TextureLoader(const aiScene* pScene, const std::string& pModelPath, TextureType pTextureType, const std::string& pTexturePath)
	: mTexturePath(pTexturePath)
	, mModelPath(pModelPath)
	, mTextureType(pTextureType)
	, mScene(pScene) { }

TextureResource* TextureLoader::loadTexture() {
	TextureResource* tTexture = nullptr;

	try {
		tTexture = createReferencedTexture();
	}
	catch (...) {
		tTexture = createEmbeddedTexture();
	}

	if (!tTexture) {
		throw "Undefined texture";
	}

	return tTexture;
}

TextureResource* TextureLoader::createEmbeddedTexture() {
	const aiTexture* tTexture = mScene->GetEmbeddedTexture(mTexturePath.c_str());

	int tWidth = -1;
	int tHeight = -1;
	int tChannelCount = -1;

	unsigned char* tTextureData = nullptr;

	if (tTexture->mHeight == 0) {
		tTextureData = stbi_load_from_memory(reinterpret_cast<unsigned char*>(tTexture->pcData), tTexture->mWidth, &tWidth, &tHeight, &tChannelCount, 0);
	}
	else {
		tTextureData = stbi_load_from_memory(reinterpret_cast<unsigned char*>(tTexture->pcData), tTexture->mWidth * tTexture->mHeight, &tWidth, &tHeight, &tChannelCount, 0);
	}

	return createTexture(tTextureData, tWidth, tHeight, tChannelCount);
}

TextureResource* TextureLoader::createReferencedTexture() {
	std::string tTexturePath = getTexturePath();

	if (tTexturePath.empty()) {
		throw "Texture path is not found.";
	}

	int tWidth = -1;
	int tHeight = -1;
	int tChannelCount = -1;

	//stbi_set_flip_vertically_on_load(true);
	unsigned char* tTextureData = stbi_load(tTexturePath.c_str(), &tWidth, &tHeight, &tChannelCount, 0);

	return createTexture(tTextureData, tWidth, tHeight, tChannelCount);
}

TextureResource* TextureLoader::createTexture(unsigned char* pTextureData, int pWidth, int pHeight, int pChannelCount) {
	unsigned int tColorMode = getColorMode(pChannelCount);

	unsigned int tTextureId;

	glGenTextures(1, &tTextureId);
	glBindTexture(GL_TEXTURE_2D, tTextureId);

	glTexImage2D(GL_TEXTURE_2D, 0, tColorMode, pWidth, pHeight, 0, tColorMode, GL_UNSIGNED_BYTE, pTextureData);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(pTextureData);

	std::cout << "Texture: ${" << mTexturePath << "} is created." << std::endl;

	return new TextureResource(tTextureId, mTexturePath);
}

unsigned int TextureLoader::getColorMode(int pChannelCount) {
	unsigned int tColorMode = GL_RGB;

	if (pChannelCount == 1) {
		tColorMode = GL_RED;
	}
	else if (pChannelCount == 2) {
		tColorMode = GL_RG;
	}
	else if (pChannelCount == 3) {
		tColorMode = GL_RGB;
	}
	else if (pChannelCount == 4) {
		tColorMode = GL_RGBA;
	}
	else {
		throw "Undefined channel count.";
	}

	return tColorMode;
}

void TextureLoader::createPossibleTextureLocations() {
	std::filesystem::path tSourceDirectory = FileManager::getInstance()->getDirectoryFromPath(mModelPath);

	mPossibleTextureLocations.push_back(tSourceDirectory.string());
	mPossibleTextureLocations.push_back(FileManager::getInstance()->createPath(tSourceDirectory, "textures"));

	if (FileManager::getInstance()->hasParentPath(tSourceDirectory)) {
		std::string tParentDirectory = FileManager::getInstance()->getParentDirectory(tSourceDirectory);
		mPossibleTextureLocations.push_back(tParentDirectory);
		mPossibleTextureLocations.push_back(FileManager::getInstance()->createPath(tParentDirectory, "textures"));
	}
}

std::string TextureLoader::getTexturePath() {
	std::string tTextureFileName = FileManager::getInstance()->getFileNameFromPath(mTexturePath);

	createPossibleTextureLocations();

	for (unsigned int i = 0; i < mPossibleTextureLocations.size(); i++) {
		if (FileManager::getInstance()->isFileExist(mPossibleTextureLocations[i], tTextureFileName)) {
			return FileManager::getInstance()->createPath(mPossibleTextureLocations[i], tTextureFileName);
		}
	}

	std::string tTextureFileNameSpacesChangedWithUnderscore = tTextureFileName;

	std::ranges::replace_if(tTextureFileNameSpacesChangedWithUnderscore.begin(), tTextureFileNameSpacesChangedWithUnderscore.end(), ::isspace, '_');

	for (unsigned int i = 0; i < mPossibleTextureLocations.size(); i++) {
		if (FileManager::getInstance()->isFileExist(mPossibleTextureLocations[i], tTextureFileNameSpacesChangedWithUnderscore)) {
			return FileManager::getInstance()->createPath(mPossibleTextureLocations[i], tTextureFileNameSpacesChangedWithUnderscore);
		}
	}

	return "";
}
