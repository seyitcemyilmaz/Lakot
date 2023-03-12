#ifndef LAKOT_TEXTURERESOURCE_H
#define LAKOT_TEXTURERESOURCE_H

#include <iostream>

class TextureResource {
private:
    unsigned int mTextureId;

    std::string mTexturePath;

public:
    TextureResource(std::string pTexturePath);
};


#endif
