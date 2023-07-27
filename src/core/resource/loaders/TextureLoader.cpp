#include "TextureLoader.h"

#include "helper/FileManager.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

TextureLoader::TextureLoader(const std::string& pModelPath, const std::string& pTexturePath) :
	mTexturePath(pTexturePath), mModelPath(pModelPath) { }

TextureResource* TextureLoader::loadTexture() {
	if ('*' == mTexturePath[0])  {
		return createEmbeddedTexture();
	}

	return createReferencedTexture();
}

TextureResource* TextureLoader::createEmbeddedTexture() {
	throw "Not implemented";
}

TextureResource* TextureLoader::createReferencedTexture() {
	std::string tTexturePath = getTexturePath();

	if (tTexturePath.empty()) {
		throw "Texture path is not found.";
	}

	int tWidth;
	int tHeight;
	int tChannels;

	stbi_set_flip_vertically_on_load(true);
	unsigned char* tTextureData = stbi_load(tTexturePath.c_str(), &tWidth, &tHeight, &tChannels, 0);

	if (!tTextureData) {
		throw "Texture data is not found.";
	}

	unsigned int tTextureId;

	glGenTextures(1, &tTextureId);
	glBindTexture(GL_TEXTURE_2D, tTextureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tWidth, tHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, tTextureData);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(tTextureData);

	std::cout << "Texture is found: " << tTexturePath << std::endl;

	return new TextureResource(tTextureId, tTexturePath);
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
