#ifndef LAKOT_TEXTURE_H
#define LAKOT_TEXTURE_H

#include <cstddef>
#include <memory>
#include <string>

namespace lakot
{

class Texture
{
public:
    static std::shared_ptr<Texture> fromFile(const std::string& pPath, std::string& pError);
    static std::shared_ptr<Texture> fromMemory(const unsigned char* pData, size_t pSize, std::string& pError);
    static std::shared_ptr<Texture> white();

    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void bind(unsigned int pTextureUnit) const;

private:
    explicit Texture(unsigned int pId);

    unsigned int mId;

    static std::shared_ptr<Texture> upload(const unsigned char* pPixels, int pWidth, int pHeight);
};

}

#endif
