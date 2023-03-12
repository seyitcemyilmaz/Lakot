#ifndef LAKOT_TEXTURELOADER_H
#define LAKOT_TEXTURELOADER_H

#include <iostream>
#include <vector>

#include "textureresource.h"

class TextureLoader {
private:
    std::string mTexturePath;
    std::string mSourcePath;

    std::vector<std::string> mPossibleTextureLocations;

    std::string getFileName(std::string pPath);
    std::string getDirectory(std::string pPath);

    void createPossibleTextureLocations(std::string pDirectory);

    bool isFileExist(std::string pDirectory, std::string pTextureFileName);
    std::string createDirectoryString(std::string pPath, std::string pDirectory);
    std::string createPath(std::string pDirectory, std::string pFileName);
public:
    TextureLoader(std::string pSourcePath, std::string pTexturePath);

    TextureResource* loadTexture();
};

#endif
