#ifndef LAKOT_ASSET_MANAGER_H
#define LAKOT_ASSET_MANAGER_H

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../graphics/model/Texture.h"

#include "AssetCache.hpp"
#include "AssetPrefetcher.h"

struct aiScene;

namespace lakot
{

class GarbageCollector;
class Model;
class SkinnedModel;
class Texture;

class AssetManager
{
public:
    AssetManager(std::string pRootDirectory, GarbageCollector& pGarbageCollector);
    ~AssetManager();

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    void prefetchModel(const std::string& pRelativePath);
    void prefetchSkinnedModel(const std::string& pRelativePath);
    void prefetchMotionSource(const std::string& pRelativePath);

    void update(double pBudgetSeconds);

    std::shared_ptr<Model> getModel(const std::string& pRelativePath);
    std::shared_ptr<SkinnedModel> getSkinnedModel(const std::string& pRelativePath);
    std::shared_ptr<SkinnedModel> getMotionSource(const std::string& pRelativePath);

    // The base colour texture of a model's first textured material, shared by every model that uses it.
    std::shared_ptr<Texture> getModelTexture(const aiScene& pScene, const std::string& pModelPath, std::string& pError);

    std::vector<std::string> list(const std::string& pRelativeDirectory, const std::string& pExtension) const;

    std::string resolve(const std::string& pRelativePath) const;

    void clear();

private:
    template <typename T, typename Builder, typename Loader>
    std::shared_ptr<T> obtain(AssetCache<T>& pCache, AssetKindType pKind, const std::string& pPath, Builder pBuilder, Loader pLoader);

    template <typename T, typename Builder>
    std::shared_ptr<T> finishParsed(ParsedAsset& pParsed, Builder pBuilder);

    std::shared_ptr<Model> buildModel(const aiScene& pScene, const std::string& pPath, std::string& pError);
    std::shared_ptr<SkinnedModel> buildSkinnedModel(const aiScene& pScene, const std::string& pPath, std::string& pError);
    std::shared_ptr<SkinnedModel> buildMotionSource(const aiScene& pScene, const std::string& pPath, std::string& pError);

    void prefetch(AssetKindType pKind, const std::string& pRelativePath);
    void finishPrefetched(ParsedAsset& pParsed);
    void sweepIdle();

    template <typename T>
    void releaseOnMainThread(std::vector<std::shared_ptr<T>>&& pDoomed);

    std::string mRootDirectory;
    GarbageCollector& mGarbageCollector;
    AssetPrefetcher mPrefetcher;

    AssetCache<Model> mModels;
    AssetCache<SkinnedModel> mSkinnedModels;
    AssetCache<SkinnedModel> mMotionSources;
    AssetCache<Texture> mTextures;

    std::unordered_map<std::string, DecodedImage> mPendingImages;
    std::chrono::steady_clock::time_point mLastSweep;
};

}

#endif
