#include "AssetManager.h"

#include <algorithm>
#include <filesystem>

#include <SDL3/SDL_log.h>

#include <assimp/scene.h>

#include "GarbageCollector.h"

#include "ModelTextureSourceLoader.h"
#include "../graphics/model/Model.h"
#include "../graphics/model/SkinnedModel.h"
#include "../graphics/model/Texture.h"

using namespace lakot;

namespace
{
    constexpr std::chrono::seconds kIdleLifetime{120};
    constexpr std::chrono::seconds kSweepInterval{5};

    template <typename T>
    void logFailure(const std::shared_ptr<T>& pAsset, const std::string& pError)
    {
        if (!pAsset)
        {
            SDL_Log("AssetManager: %s", pError.c_str());
        }
    }
}

AssetManager::AssetManager(std::string pRootDirectory, GarbageCollector& pGarbageCollector)
    : mRootDirectory(std::move(pRootDirectory))
    , mGarbageCollector(pGarbageCollector)
    , mPrefetcher(pGarbageCollector)
    , mLastSweep(std::chrono::steady_clock::now())
{

}

AssetManager::~AssetManager()
{
    clear();
    mGarbageCollector.executeSynchronousTasks();
}

std::string AssetManager::resolve(const std::string& pRelativePath) const
{
    return (std::filesystem::path(mRootDirectory) / pRelativePath).lexically_normal().string();
}

template <typename T, typename Builder>
std::shared_ptr<T> AssetManager::finishParsed(ParsedAsset& pParsed, Builder pBuilder)
{
    std::string tError = pParsed.error;
    std::shared_ptr<T> tAsset;

    if (pParsed.scene)
    {
        mPendingImages = std::move(pParsed.images);
        tAsset = pBuilder(*pParsed.scene, tError);
        mPendingImages.clear();
    }

    logFailure(tAsset, tError);
    return tAsset;
}

template <typename T, typename Builder, typename Loader>
std::shared_ptr<T> AssetManager::obtain(AssetCache<T>& pCache, AssetKindType pKind, const std::string& pPath, Builder pBuilder, Loader pLoader)
{
    if (const std::shared_ptr<T>* tFound = pCache.find(pPath))
    {
        return *tFound;
    }

    std::shared_ptr<T> tAsset;

    if (std::optional<ParsedAsset> tParsed = mPrefetcher.take(pKind, pPath))
    {
        tAsset = finishParsed<T>(*tParsed, pBuilder);
    }
    else
    {
        std::string tError;
        tAsset = pLoader(tError);
        logFailure(tAsset, tError);
    }

    pCache.insert(pPath, tAsset);
    return tAsset;
}

std::shared_ptr<Model> AssetManager::buildModel(const aiScene& pScene, const std::string& pPath, std::string& pError)
{
    std::shared_ptr<Model> tModel = Model::build(pScene, pPath, *this, pError);

    if (tModel)
    {
        tModel->initialize();
    }

    return tModel;
}

std::shared_ptr<SkinnedModel> AssetManager::buildSkinnedModel(const aiScene& pScene, const std::string& pPath, std::string& pError)
{
    std::shared_ptr<SkinnedModel> tModel = SkinnedModel::build(pScene, pPath, *this, pError);

    if (tModel)
    {
        tModel->initialize();
    }

    return tModel;
}

std::shared_ptr<SkinnedModel> AssetManager::buildMotionSource(const aiScene& pScene, const std::string& pPath, std::string& pError)
{
    return SkinnedModel::build(pScene, pPath, *this, pError);
}

std::shared_ptr<Model> AssetManager::getModel(const std::string& pRelativePath)
{
    std::string tPath = resolve(pRelativePath);

    return obtain(mModels, AssetKindType::eModel, tPath,
        [this, &tPath](const aiScene& pScene, std::string& pError) { return buildModel(pScene, tPath, pError); },
        [this, &tPath](std::string& pError) -> std::shared_ptr<Model>
        {
            std::shared_ptr<Model> tModel = Model::load(tPath, *this, pError);

            if (tModel)
            {
                tModel->initialize();
            }

            return tModel;
        });
}

std::shared_ptr<SkinnedModel> AssetManager::getSkinnedModel(const std::string& pRelativePath)
{
    std::string tPath = resolve(pRelativePath);

    return obtain(mSkinnedModels, AssetKindType::eSkinnedModel, tPath,
        [this, &tPath](const aiScene& pScene, std::string& pError) { return buildSkinnedModel(pScene, tPath, pError); },
        [this, &tPath](std::string& pError) -> std::shared_ptr<SkinnedModel>
        {
            std::shared_ptr<SkinnedModel> tModel = SkinnedModel::load(tPath, *this, pError);

            if (tModel)
            {
                tModel->initialize();
            }

            return tModel;
        });
}

std::shared_ptr<SkinnedModel> AssetManager::getMotionSource(const std::string& pRelativePath)
{
    std::string tPath = resolve(pRelativePath);

    return obtain(mMotionSources, AssetKindType::eMotionSource, tPath,
        [this, &tPath](const aiScene& pScene, std::string& pError) { return buildMotionSource(pScene, tPath, pError); },
        [this, &tPath](std::string& pError) -> std::shared_ptr<SkinnedModel>
        {
            return SkinnedModel::load(tPath, *this, pError);
        });
}

void AssetManager::prefetchModel(const std::string& pRelativePath)
{
    prefetch(AssetKindType::eModel, pRelativePath);
}

void AssetManager::prefetchSkinnedModel(const std::string& pRelativePath)
{
    prefetch(AssetKindType::eSkinnedModel, pRelativePath);
}

void AssetManager::prefetchMotionSource(const std::string& pRelativePath)
{
    prefetch(AssetKindType::eMotionSource, pRelativePath);
}

void AssetManager::prefetch(AssetKindType pKind, const std::string& pRelativePath)
{
    std::string tPath = resolve(pRelativePath);

    bool tIsCached = pKind == AssetKindType::eModel ? mModels.contains(tPath)
        : pKind == AssetKindType::eSkinnedModel ? mSkinnedModels.contains(tPath)
        : mMotionSources.contains(tPath);

    if (tIsCached)
    {
        return;
    }

    unsigned int tFlags = pKind == AssetKindType::eModel ? Model::getImportFlags() : SkinnedModel::getImportFlags();
    mPrefetcher.request(pKind, tPath, tFlags);
}

void AssetManager::update(double pBudgetSeconds)
{
    std::chrono::steady_clock::time_point tStart = std::chrono::steady_clock::now();

    while (std::optional<ParsedAsset> tParsed = mPrefetcher.takeAny())
    {
        finishPrefetched(*tParsed);

        if (std::chrono::duration<double>(std::chrono::steady_clock::now() - tStart).count() >= pBudgetSeconds)
        {
            break;
        }
    }

    if (tStart - mLastSweep >= kSweepInterval)
    {
        mLastSweep = tStart;
        sweepIdle();
    }
}

void AssetManager::finishPrefetched(ParsedAsset& pParsed)
{
    const std::string& tPath = pParsed.path;

    switch (pParsed.kind)
    {
    case AssetKindType::eModel:
        if (!mModels.contains(tPath))
        {
            mModels.insert(tPath, finishParsed<Model>(pParsed, [this, &tPath](const aiScene& pScene, std::string& pError) { return buildModel(pScene, tPath, pError); }));
        }
        break;
    case AssetKindType::eSkinnedModel:
        if (!mSkinnedModels.contains(tPath))
        {
            mSkinnedModels.insert(tPath, finishParsed<SkinnedModel>(pParsed, [this, &tPath](const aiScene& pScene, std::string& pError) { return buildSkinnedModel(pScene, tPath, pError); }));
        }
        break;
    case AssetKindType::eMotionSource:
        if (!mMotionSources.contains(tPath))
        {
            mMotionSources.insert(tPath, finishParsed<SkinnedModel>(pParsed, [this, &tPath](const aiScene& pScene, std::string& pError) { return buildMotionSource(pScene, tPath, pError); }));
        }
        break;
    }
}

template <typename T>
void AssetManager::releaseOnMainThread(std::vector<std::shared_ptr<T>>&& pDoomed)
{
    if (pDoomed.empty())
    {
        return;
    }

    std::shared_ptr<std::vector<std::shared_ptr<T>>> tDoomed = std::make_shared<std::vector<std::shared_ptr<T>>>(std::move(pDoomed));

    mGarbageCollector.addSynchronousTask([tDoomed]()
    {
        tDoomed->clear();
    });
}

void AssetManager::sweepIdle()
{
    std::chrono::steady_clock::time_point tNow = std::chrono::steady_clock::now();

    releaseOnMainThread(mModels.takeIdle(tNow, kIdleLifetime));
    releaseOnMainThread(mTextures.takeIdle(tNow, kIdleLifetime));
}

std::shared_ptr<Texture> AssetManager::getModelTexture(const aiScene& pScene, const std::string& pModelPath, std::string& pError)
{
    for (const ModelTextureSource& tSource : ModelTextureSourceLoader::list(pScene, pModelPath))
    {
        std::shared_ptr<Texture> tTexture;

        if (const std::shared_ptr<Texture>* tFound = mTextures.find(tSource.key))
        {
            tTexture = *tFound;
        }
        else
        {
            std::string tLoadError;
            auto tPending = mPendingImages.find(tSource.key);

            if (tPending != mPendingImages.end())
            {
                tTexture = Texture::fromImage(tPending->second);
            }
            else if (!tSource.embedded)
            {
                tTexture = Texture::fromFile(tSource.key, tLoadError);
            }
            else if (tSource.embedded->mHeight != 0)
            {
                tLoadError = "uncompressed embedded textures are not supported";
            }
            else
            {
                tTexture = Texture::fromMemory(reinterpret_cast<const unsigned char*>(tSource.embedded->pcData), tSource.embedded->mWidth, tLoadError);
            }

            logFailure(tTexture, tLoadError);
            mTextures.insert(tSource.key, tTexture);
        }

        if (tTexture)
        {
            return tTexture;
        }

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Cannot read texture '%s' of %s, using white", tSource.name.c_str(), pModelPath.c_str());
    }

    if (const std::shared_ptr<Texture>* tWhite = mTextures.find("#white"))
    {
        return *tWhite;
    }

    std::shared_ptr<Texture> tWhite = Texture::white();
    mTextures.insert("#white", tWhite);
    return tWhite;
}

std::vector<std::string> AssetManager::list(const std::string& pRelativeDirectory, const std::string& pExtension) const
{
    std::vector<std::string> tPaths;
    std::error_code tError;

    for (const auto& tEntry : std::filesystem::directory_iterator(resolve(pRelativeDirectory), tError))
    {
        if (tEntry.path().extension() == pExtension)
        {
            tPaths.push_back((std::filesystem::path(pRelativeDirectory) / tEntry.path().filename()).generic_string());
        }
    }

    std::sort(tPaths.begin(), tPaths.end());
    return tPaths;
}

void AssetManager::clear()
{
    mModels.clear();
    mSkinnedModels.clear();
    mMotionSources.clear();
    mTextures.clear();
}
