#include "textureloader.h"

#include "helper/FileManager.h"

TextureLoader::TextureLoader(std::string pSourcePath, std::string pTexturePath) {
	mTexturePath = pTexturePath;
	mSourcePath = pSourcePath;
}

TextureResource* TextureLoader::loadTexture() {
	std::string tTexturePath = getTexturePath();

	if (tTexturePath.empty()) {
		std::cout << "TEXTURE BULUNAMADI: " << mTexturePath << std::endl;
		return nullptr;
	}

	std::cout << "TEXTURE BULUNDU: " << tTexturePath << std::endl;


	return nullptr;
}

void TextureLoader::createPossibleTextureLocations() {
	std::string tSourceDirectory = FileManager::getInstance()->getDirectoryFromPath(mSourcePath);

	mPossibleTextureLocations.push_back(tSourceDirectory);
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

	std::replace_if(tTextureFileNameSpacesChangedWithUnderscore.begin(),
					tTextureFileNameSpacesChangedWithUnderscore.end(),
					::isspace,
					'_');


	for (unsigned int i = 0; i < mPossibleTextureLocations.size(); i++) {
		if (FileManager::getInstance()->isFileExist(mPossibleTextureLocations[i], tTextureFileNameSpacesChangedWithUnderscore)) {
			return FileManager::getInstance()->createPath(mPossibleTextureLocations[i], tTextureFileNameSpacesChangedWithUnderscore);
		}
	}

	return "";
}
