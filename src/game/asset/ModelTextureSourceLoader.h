#ifndef LAKOT_MODEL_TEXTURE_SOURCE_LOADER_H
#define LAKOT_MODEL_TEXTURE_SOURCE_LOADER_H

#include <string>
#include <vector>

struct aiScene;
struct aiTexture;

namespace lakot
{

struct ModelTextureSource
{
    std::string key;
    std::string name;
    const aiTexture* embedded{nullptr};
};

class ModelTextureSourceLoader
{
public:
    static std::vector<ModelTextureSource> list(const aiScene& pScene, const std::string& pModelPath);

private:
    ModelTextureSourceLoader() = delete;
    ~ModelTextureSourceLoader() = delete;
};

}

#endif
