#include "AssetManager.h"

#include <algorithm>
#include <filesystem>

#include <SDL3/SDL_log.h>

#include <assimp/scene.h>

#include "../graphics/model/Model.h"
#include "../graphics/model/SkinnedModel.h"
#include "../graphics/model/Texture.h"

using namespace lakot;

namespace
{
    template <typename T, typename Loader>
    std::shared_ptr<T> getCached(std::unordered_map<std::string, std::shared_ptr<T>>& pCache, const std::string& pKey, Loader pLoader)
    {
        auto tIterator = pCache.find(pKey);

        if (tIterator != pCache.end())
        {
            return tIterator->second;
        }

        std::string tError;
        std::shared_ptr<T> tAsset = pLoader(tError);

        if (!tAsset)
        {
            SDL_Log("AssetManager: %s", tError.c_str());
        }

        pCache.emplace(pKey, tAsset);
        return tAsset;
    }
}

AssetManager::AssetManager(std::string pRootDirectory)
    : mRootDirectory(std::move(pRootDirectory))
{

}

AssetManager::~AssetManager()
{
    clear();
}

std::string AssetManager::resolve(const std::string& pRelativePath) const
{
    return (std::filesystem::path(mRootDirectory) / pRelativePath).lexically_normal().string();
}

std::shared_ptr<Model> AssetManager::getModel(const std::string& pRelativePath)
{
    std::string tPath = resolve(pRelativePath);

    return getCached(mModels, tPath, [this, &tPath](std::string& pError) -> std::shared_ptr<Model>
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

    return getCached(mSkinnedModels, tPath, [this, &tPath](std::string& pError) -> std::shared_ptr<SkinnedModel>
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

    return getCached(mMotionSources, tPath, [this, &tPath](std::string& pError) -> std::shared_ptr<SkinnedModel>
    {
        return SkinnedModel::load(tPath, *this, pError);
    });
}

std::shared_ptr<Texture> AssetManager::getModelTexture(const aiScene& pScene, const std::string& pModelPath, std::string& pError)
{
    for (unsigned int tMaterialIndex = 0; tMaterialIndex < pScene.mNumMaterials; ++tMaterialIndex)
    {
        const aiMaterial* tMaterial = pScene.mMaterials[tMaterialIndex];
        aiString tTexturePath;

        if (tMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &tTexturePath) != AI_SUCCESS
            && tMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &tTexturePath) != AI_SUCCESS)
        {
            continue;
        }

        std::shared_ptr<Texture> tTexture;

        if (const aiTexture* tEmbedded = pScene.GetEmbeddedTexture(tTexturePath.C_Str()))
        {
            std::string tKey = pModelPath + "#" + tTexturePath.C_Str();

            tTexture = getCached(mTextures, tKey, [tEmbedded](std::string& pLoadError) -> std::shared_ptr<Texture>
            {
                if (tEmbedded->mHeight != 0)
                {
                    pLoadError = "uncompressed embedded textures are not supported";
                    return nullptr;
                }

                return Texture::fromMemory(reinterpret_cast<const unsigned char*>(tEmbedded->pcData), tEmbedded->mWidth, pLoadError);
            });
        }
        else
        {
            std::string tKey = (std::filesystem::path(pModelPath).parent_path() / tTexturePath.C_Str()).lexically_normal().string();

            tTexture = getCached(mTextures, tKey, [&tKey](std::string& pLoadError)
            {
                return Texture::fromFile(tKey, pLoadError);
            });
        }

        if (!tTexture)
        {
            pError = "cannot read texture '" + std::string(tTexturePath.C_Str()) + "' of " + pModelPath;
        }

        return tTexture;
    }

    return getCached(mTextures, "#white", [](std::string&)
    {
        return Texture::white();
    });
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
