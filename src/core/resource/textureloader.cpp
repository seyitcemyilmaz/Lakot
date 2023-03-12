#include "textureloader.h"

#include <filesystem>

TextureLoader::TextureLoader(std::string pSourcePath, std::string pTexturePath) {
    mTexturePath = pTexturePath;
    mSourcePath = pSourcePath;
}

TextureResource* TextureLoader::loadTexture() {
    std::string tTextureFileName = getFileName(mTexturePath);
    std::string tSourceDirectory = getDirectory(mSourcePath);

    createPossibleTextureLocations(tSourceDirectory);

    for (unsigned int i = 0; i < mPossibleTextureLocations.size(); i++) {
        if (isFileExist(mPossibleTextureLocations[i], tTextureFileName)) {
            // TODO: Will be continued.
            std::cout << createPath(mPossibleTextureLocations[i], tTextureFileName) << std::endl;
        }
    }

    std::string tTextureFileNameSpacesChangedWithUnderscore = tTextureFileName;

    std::replace_if(tTextureFileNameSpacesChangedWithUnderscore.begin(),
                    tTextureFileNameSpacesChangedWithUnderscore.end(),
                    ::isspace,
                    '_');


    for (unsigned int i = 0; i < mPossibleTextureLocations.size(); i++) {
        if (isFileExist(mPossibleTextureLocations[i], tTextureFileNameSpacesChangedWithUnderscore)) {
            // TODO: Will be continued.
            std::cout << createPath(mPossibleTextureLocations[i], tTextureFileNameSpacesChangedWithUnderscore) << std::endl;
        }
    }

    return nullptr;
}

void TextureLoader::createPossibleTextureLocations(std::string pDirectory) {
    std::filesystem::path tBaseDirectory = pDirectory;

    mPossibleTextureLocations.push_back(tBaseDirectory.string());
    mPossibleTextureLocations.push_back(createDirectoryString(tBaseDirectory.string(), "textures"));

    if (tBaseDirectory.has_parent_path()) {
        std::filesystem::path tParentDirectory = tBaseDirectory.parent_path();
        mPossibleTextureLocations.push_back(tParentDirectory.string());
        mPossibleTextureLocations.push_back(createDirectoryString(tParentDirectory.string(), "textures"));
    }
}

bool TextureLoader::isFileExist(std::string pDirectory, std::string pTextureFileName) {
    return std::filesystem::exists(pDirectory + "/" + pTextureFileName);
}

std::string TextureLoader::createDirectoryString(std::string pPath, std::string pDirectory) {
    return pPath + "/" + pDirectory;
}

std::string TextureLoader::createPath(std::string pDirectory, std::string pFileName) {
    return pDirectory + "/" + pFileName;
}

std::string TextureLoader::getFileName(std::string pPath) {
    return pPath.substr(pPath.find_last_of("/\\") + 1);
}

std::string TextureLoader::getDirectory(std::string pPath) {
    return pPath.substr(0, pPath.find_last_of("/\\"));
}
