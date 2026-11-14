#include "TextureArray.h"

#include <SDL3/SDL_log.h>
#include <glad/glad.h>

#include "Texture.h"

using namespace lakot;

TextureArray::TextureArray(unsigned int pId)
    : mId(pId)
{

}

TextureArray::~TextureArray()
{
    glDeleteTextures(1, &mId);
}

std::unique_ptr<TextureArray> TextureArray::fromFiles(const std::vector<std::string>& pPaths)
{
    if (pPaths.empty())
    {
        return nullptr;
    }

    const int tLayerCount = static_cast<int>(pPaths.size());
    const size_t tLayerBytes = static_cast<size_t>(kLayerSize) * kLayerSize * 4;

    int tLevels = 1;

    for (int tSize = kLayerSize; tSize > 1; tSize /= 2)
    {
        tLevels++;
    }

    unsigned int tId = 0;
    glGenTextures(1, &tId);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tId);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, tLevels, GL_RGBA8, kLayerSize, kLayerSize, tLayerCount);

    for (int tLayer = 0; tLayer < tLayerCount; tLayer++)
    {
        int tWidth = 0;
        int tHeight = 0;
        std::string tError;
        std::vector<unsigned char> tPixels = Texture::readRgba(pPaths[tLayer], tWidth, tHeight, tError);

        if (tPixels.empty() || tWidth != kLayerSize || tHeight != kLayerSize)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "TextureArray: %s, using a white layer",
                         tPixels.empty() ? tError.c_str() : (pPaths[tLayer] + " is not 128x128").c_str());
            tPixels.assign(tLayerBytes, 255);
        }

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, tLayer, kLayerSize, kLayerSize, 1, GL_RGBA, GL_UNSIGNED_BYTE, tPixels.data());
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    return std::unique_ptr<TextureArray>(new TextureArray(tId));
}

void TextureArray::bind(unsigned int pTextureUnit) const
{
    glActiveTexture(GL_TEXTURE0 + pTextureUnit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, mId);
}
