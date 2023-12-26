#ifndef LAKOT_TEXTURE_H
#define LAKOT_TEXTURE_H

#include "core/resource/TextureResource.h"

class Texture {
private:
    TextureResource* mTextureResource;

public:
    Texture(TextureResource* pTextureResource);
};

#endif
