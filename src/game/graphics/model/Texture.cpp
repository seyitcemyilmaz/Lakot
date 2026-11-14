#include "Texture.h"

#include <algorithm>
#include <limits>

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

namespace
{
    constexpr float kMaxAnisotropy = 8.0f;
}

Texture::Texture(unsigned int pId)
    : mId(pId)
{

}

Texture::~Texture()
{
    glDeleteTextures(1, &mId);
}

std::vector<unsigned char> Texture::readRgba(const std::string& pPath, int& pWidth, int& pHeight, std::string& pError)
{
    DecodedImage tImage = decodeFile(pPath, pError);
    pWidth = tImage.width;
    pHeight = tImage.height;
    return std::move(tImage.pixels);
}

std::shared_ptr<Texture> Texture::fromFile(const std::string& pPath, std::string& pError, TextureFilterType pFilter, TextureWrapType pWrap)
{
    return fromImage(decodeFile(pPath, pError), pFilter, pWrap);
}

DecodedImage Texture::decodeFile(const std::string& pPath, std::string& pError)
{
    DecodedImage tImage;
    int tChannels = 0;

    stbi_set_flip_vertically_on_load_thread(1);
    stbi_uc* tPixels = stbi_load(pPath.c_str(), &tImage.width, &tImage.height, &tChannels, 4);

    if (!tPixels)
    {
        pError = "cannot read texture " + pPath;
        return {};
    }

    tImage.pixels.assign(tPixels, tPixels + static_cast<size_t>(tImage.width) * static_cast<size_t>(tImage.height) * 4);
    stbi_image_free(tPixels);
    return tImage;
}

DecodedImage Texture::decodeMemory(const unsigned char* pData, size_t pSize, std::string& pError)
{
    DecodedImage tImage;
    int tChannels = 0;

    if (!pData || pSize == 0 || pSize > static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        pError = "embedded texture has no data or is too large";
        return {};
    }

    stbi_set_flip_vertically_on_load_thread(1);
    stbi_uc* tPixels = stbi_load_from_memory(pData, static_cast<int>(pSize), &tImage.width, &tImage.height, &tChannels, 4);

    if (!tPixels)
    {
        pError = "cannot decode embedded texture";
        return {};
    }

    tImage.pixels.assign(tPixels, tPixels + static_cast<size_t>(tImage.width) * static_cast<size_t>(tImage.height) * 4);
    stbi_image_free(tPixels);
    return tImage;
}

std::shared_ptr<Texture> Texture::fromImage(const DecodedImage& pImage, TextureFilterType pFilter, TextureWrapType pWrap)
{
    if (pImage.pixels.empty())
    {
        return nullptr;
    }

    return upload(pImage.pixels.data(), pImage.width, pImage.height, pFilter, pWrap);
}

std::shared_ptr<Texture> Texture::fromMemory(const unsigned char* pData, size_t pSize, std::string& pError)
{
    return fromImage(decodeMemory(pData, pSize, pError));
}

std::shared_ptr<Texture> Texture::white()
{
    const unsigned char tPixel[4] = { 255, 255, 255, 255 };
    return upload(tPixel, 1, 1, TextureFilterType::eNearest, TextureWrapType::eRepeat);
}

std::shared_ptr<Texture> Texture::upload(const unsigned char* pPixels, int pWidth, int pHeight, TextureFilterType pFilter, TextureWrapType pWrap)
{
    unsigned int tId = 0;

    glGenTextures(1, &tId);
    glBindTexture(GL_TEXTURE_2D, tId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, pWidth, pHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pPixels);

    if (pFilter == TextureFilterType::eTrilinear)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        float tMaxAnisotropy = 1.0f;

        while (glGetError() != GL_NO_ERROR)
        {
        }

        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &tMaxAnisotropy);

        if (glGetError() == GL_NO_ERROR && tMaxAnisotropy > 1.0f)
        {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, std::min(tMaxAnisotropy, kMaxAnisotropy));
        }
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    const int tWrap = pWrap == TextureWrapType::eClamp ? GL_CLAMP_TO_EDGE : GL_REPEAT;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tWrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWrap);
    glBindTexture(GL_TEXTURE_2D, 0);

    return std::shared_ptr<Texture>(new Texture(tId));
}

void Texture::bind(unsigned int pTextureUnit) const
{
    glActiveTexture(GL_TEXTURE0 + pTextureUnit);
    glBindTexture(GL_TEXTURE_2D, mId);
}
