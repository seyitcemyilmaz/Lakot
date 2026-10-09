#include "Texture.h"

#include <glad/glad.h>

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include <stb_image.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

using namespace lakot;

Texture::Texture(unsigned int pId)
    : mId(pId)
{

}

Texture::~Texture()
{
    glDeleteTextures(1, &mId);
}

std::shared_ptr<Texture> Texture::fromFile(const std::string& pPath, std::string& pError)
{
    int tWidth = 0;
    int tHeight = 0;
    int tChannels = 0;

    stbi_set_flip_vertically_on_load(1);
    stbi_uc* tPixels = stbi_load(pPath.c_str(), &tWidth, &tHeight, &tChannels, 4);

    if (!tPixels)
    {
        pError = "cannot read texture " + pPath;
        return nullptr;
    }

    std::shared_ptr<Texture> tTexture = upload(tPixels, tWidth, tHeight);
    stbi_image_free(tPixels);
    return tTexture;
}

std::shared_ptr<Texture> Texture::fromMemory(const unsigned char* pData, size_t pSize, std::string& pError)
{
    int tWidth = 0;
    int tHeight = 0;
    int tChannels = 0;

    stbi_set_flip_vertically_on_load(1);
    stbi_uc* tPixels = stbi_load_from_memory(pData, static_cast<int>(pSize), &tWidth, &tHeight, &tChannels, 4);

    if (!tPixels)
    {
        pError = "cannot decode embedded texture";
        return nullptr;
    }

    std::shared_ptr<Texture> tTexture = upload(tPixels, tWidth, tHeight);
    stbi_image_free(tPixels);
    return tTexture;
}

std::shared_ptr<Texture> Texture::white()
{
    const unsigned char tPixel[4] = { 255, 255, 255, 255 };
    return upload(tPixel, 1, 1);
}

std::shared_ptr<Texture> Texture::upload(const unsigned char* pPixels, int pWidth, int pHeight)
{
    unsigned int tId = 0;

    glGenTextures(1, &tId);
    glBindTexture(GL_TEXTURE_2D, tId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, pWidth, pHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pPixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    return std::shared_ptr<Texture>(new Texture(tId));
}

void Texture::bind(unsigned int pTextureUnit) const
{
    glActiveTexture(GL_TEXTURE0 + pTextureUnit);
    glBindTexture(GL_TEXTURE_2D, mId);
}
