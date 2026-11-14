#ifndef LAKOT_TEXTURE_H
#define LAKOT_TEXTURE_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace lakot
{

enum class TextureFilterType
{
    eNearest,
    eTrilinear
};

enum class TextureWrapType
{
    eRepeat,
    eClamp
};

struct DecodedImage
{
    int width{0};
    int height{0};
    std::vector<unsigned char> pixels;
};

class Texture
{
public:
    static DecodedImage decodeFile(const std::string& pPath, std::string& pError);
    static DecodedImage decodeMemory(const unsigned char* pData, size_t pSize, std::string& pError);
    static std::shared_ptr<Texture> fromImage(const DecodedImage& pImage,
                                              TextureFilterType pFilter = TextureFilterType::eNearest,
                                              TextureWrapType pWrap = TextureWrapType::eRepeat);
    static std::vector<unsigned char> readRgba(const std::string& pPath, int& pWidth, int& pHeight, std::string& pError);
    static std::shared_ptr<Texture> fromFile(const std::string& pPath, std::string& pError,
                                             TextureFilterType pFilter = TextureFilterType::eNearest,
                                             TextureWrapType pWrap = TextureWrapType::eRepeat);
    static std::shared_ptr<Texture> fromMemory(const unsigned char* pData, size_t pSize, std::string& pError);
    static std::shared_ptr<Texture> white();

    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void bind(unsigned int pTextureUnit) const;

private:
    explicit Texture(unsigned int pId);

    unsigned int mId;

    static std::shared_ptr<Texture> upload(const unsigned char* pPixels, int pWidth, int pHeight, TextureFilterType pFilter, TextureWrapType pWrap);
};

}

#endif
