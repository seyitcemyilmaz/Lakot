#ifndef LAKOT_TEXTURE_ARRAY_H
#define LAKOT_TEXTURE_ARRAY_H

#include <memory>
#include <string>
#include <vector>

namespace lakot
{

class TextureArray
{
public:
    static constexpr int kLayerSize = 128;

    static std::unique_ptr<TextureArray> fromFiles(const std::vector<std::string>& pPaths);

    ~TextureArray();

    TextureArray(const TextureArray&) = delete;
    TextureArray& operator=(const TextureArray&) = delete;

    void bind(unsigned int pTextureUnit) const;

private:
    explicit TextureArray(unsigned int pId);

    unsigned int mId;
};

}

#endif
