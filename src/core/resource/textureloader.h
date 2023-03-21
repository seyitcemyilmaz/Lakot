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

    void createPossibleTextureLocations();

    std::string getTexturePath();
public:
    TextureLoader(std::string pSourcePath, std::string pTexturePath);

    TextureResource* loadTexture();
};

#endif
