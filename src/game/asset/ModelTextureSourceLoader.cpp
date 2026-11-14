#include "ModelTextureSourceLoader.h"

#include <filesystem>

#include <assimp/scene.h>

using namespace lakot;

std::vector<ModelTextureSource> ModelTextureSourceLoader::list(const aiScene& pScene, const std::string& pModelPath)
{
    std::vector<ModelTextureSource> tSources;

    for (unsigned int tMaterialIndex = 0; tMaterialIndex < pScene.mNumMaterials; ++tMaterialIndex)
    {
        const aiMaterial* tMaterial = pScene.mMaterials[tMaterialIndex];
        aiString tTexturePath;

        if (tMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &tTexturePath) != AI_SUCCESS
            && tMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &tTexturePath) != AI_SUCCESS)
        {
            continue;
        }

        ModelTextureSource tSource;
        tSource.name = tTexturePath.C_Str();
        tSource.embedded = pScene.GetEmbeddedTexture(tTexturePath.C_Str());

        if (tSource.embedded)
        {
            tSource.key = pModelPath + "#" + tTexturePath.C_Str();
        }
        else
        {
            tSource.key = (std::filesystem::path(pModelPath).parent_path() / tTexturePath.C_Str()).lexically_normal().string();
        }

        tSources.push_back(std::move(tSource));
    }

    return tSources;
}
