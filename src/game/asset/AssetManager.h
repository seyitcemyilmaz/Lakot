#ifndef LAKOT_ASSET_MANAGER_H
#define LAKOT_ASSET_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct aiScene;

namespace lakot
{

class Model;
class SkinnedModel;
class Texture;

class AssetManager
{
public:
    explicit AssetManager(std::string pRootDirectory);
    ~AssetManager();

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    std::shared_ptr<Model> getModel(const std::string& pRelativePath);
    std::shared_ptr<SkinnedModel> getSkinnedModel(const std::string& pRelativePath);
    std::shared_ptr<SkinnedModel> getMotionSource(const std::string& pRelativePath);

    // The base colour texture of a model's first textured material, shared by every model that uses it.
    std::shared_ptr<Texture> getModelTexture(const aiScene& pScene, const std::string& pModelPath, std::string& pError);

    std::vector<std::string> list(const std::string& pRelativeDirectory, const std::string& pExtension) const;

    std::string resolve(const std::string& pRelativePath) const;

    void clear();

private:
    std::string mRootDirectory;

    std::unordered_map<std::string, std::shared_ptr<Model>> mModels;
    std::unordered_map<std::string, std::shared_ptr<SkinnedModel>> mSkinnedModels;
    std::unordered_map<std::string, std::shared_ptr<SkinnedModel>> mMotionSources;
    std::unordered_map<std::string, std::shared_ptr<Texture>> mTextures;
};

}

#endif
