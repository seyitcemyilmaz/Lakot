#include "ModelLoader.h"

#include <spdlog/spdlog.h>

#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/version.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <lakot/utilities/FileManager.h>

#include "../asset/BoneAsset.h"
#include "../asset/ModelAsset.h"
#include "../asset/MeshAsset.h"
#include "../asset/NodeAsset.h"
#include "../asset/MaterialAsset.h"
#include "../asset/TextureAsset.h"

using namespace lakot;

ModelLoader::~ModelLoader()
{
    mImporter->FreeScene();
    delete mImporter;
}

ModelLoader::ModelLoader()
    : mModelAsset(nullptr)
    , mSceneObject(nullptr)
    , mImporter(nullptr)
{

}

ModelAsset* ModelLoader::load()
{
    unsigned int tModelImportFlags = aiProcessPreset_TargetRealtime_MaxQuality |
                                     aiProcess_FlipUVs |
                                     aiProcess_SplitByBoneCount |
                                     aiProcess_GenBoundingBoxes;

    mImporter = new Assimp::Importer();

    mImporter->SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    mImporter->SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

    mSceneObject = mImporter->ReadFile(mPath, tModelImportFlags);

    if (!mSceneObject)
    {
        mImporter->FreeScene();
        delete mSceneObject;
        return nullptr;
    }

    mModelAsset = new ModelAsset(mPath);

    loadNodeStructure();
    loadMaterials();

    return mModelAsset;
}

void ModelLoader::setPath(const std::string& pPath)
{
    mPath = pPath;
}

void ModelLoader::loadNodeStructure()
{
    auto* tRootNode = mSceneObject->mRootNode;

    if (!tRootNode)
    {
        spdlog::error("Root node is not found.");
        return;
    }

    loadNode(mSceneObject->mRootNode, nullptr);
}

void ModelLoader::loadMaterials()
{
    if (!mSceneObject->HasMaterials())
    {
        return;
    }

    createPossibleTextureLocations();

    unsigned int tMaterialCount = mSceneObject->mNumMaterials;

    for (unsigned int i = 0; i < tMaterialCount; i++)
    {
        aiMaterial* tMaterialObject = mSceneObject->mMaterials[i];
        loadMaterial(tMaterialObject);
    }
}

void ModelLoader::loadNode(aiNode* pNode, NodeAsset* pParentNodeAsset)
{
    std::string pNodeName = pNode->mName.C_Str();

    aiMatrix4x4& tMatrix = pNode->mTransformation;

    // if (pNode->mMetaData)
    // {
    //     spdlog::info("Meta: {0}",pNode->mMetaData->mNumProperties);
    // }

    glm::mat4 tTransform
    {
        tMatrix.a1, tMatrix.b1, tMatrix.c1, tMatrix.d1,
        tMatrix.a2, tMatrix.b2, tMatrix.c2, tMatrix.d2,
        tMatrix.a3, tMatrix.b3, tMatrix.c3, tMatrix.d3,
        tMatrix.a4, tMatrix.b4, tMatrix.c4, tMatrix.d4
    };

    NodeAsset* tNode = new NodeAsset(pNodeName, pParentNodeAsset, tTransform);

    if (!pParentNodeAsset)
    {
        mModelAsset->mRootNode = tNode;
    }
    else
    {
        pParentNodeAsset->addChildNode(tNode);
    }

    unsigned int tMeshCount = pNode->mNumMeshes;

    for (unsigned int i = 0; i < tMeshCount; i++)
    {
        unsigned int tMeshIndice = pNode->mMeshes[i];
        aiMesh* tMeshObject = mSceneObject->mMeshes[tMeshIndice];

        if (!tMeshObject)
        {
            spdlog::error("Mesh object is not found.");
            continue;
        }

        loadMesh(tMeshObject, tNode);
    }

    unsigned int tChildNodeCount = pNode->mNumChildren;

    for (unsigned int i = 0; i < tChildNodeCount; i++)
    {
        loadNode(pNode->mChildren[i], tNode);
    }
}

void ModelLoader::loadMesh(aiMesh* pMesh, NodeAsset* pParentNodeAsset)
{
    std::string tMeshName = pMesh->mName.C_Str();
    unsigned int tMaterialIndex = pMesh->mMaterialIndex;

    auto& tMin = pMesh->mAABB.mMin;
    auto& tMax = pMesh->mAABB.mMax;

    BoundingBox tBoundingBox(glm::vec3(tMin.x, tMin.y, tMin.z),
                             glm::vec3(tMax.x, tMax.y, tMax.z));

    MeshAsset* tMeshAsset = new MeshAsset(tMeshName, tMaterialIndex, std::move(tBoundingBox));
    pParentNodeAsset->mChildMeshes.push_back(tMeshAsset);

    DataContainer* tVertexInformation = createVertexInformation(pMesh, tMeshAsset);
    tMeshAsset->setVertexInformation(tVertexInformation);
}

void ModelLoader::loadMaterial(aiMaterial* pMaterial)
{
    MaterialAsset* tMaterialAsset = new MaterialAsset();

    {
        std::string tName = pMaterial->GetName().C_Str();
        tMaterialAsset->setName(tName);
    }

    {
        aiColor3D tColor;

        if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, tColor) == AI_SUCCESS)
        {
            tMaterialAsset->setDiffuseColor({tColor.r, tColor.g, tColor.b});
        }

        if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, tColor) == AI_SUCCESS)
        {
            tMaterialAsset->setSpecularColor({tColor.r, tColor.g, tColor.b});
        }

        if (pMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, tColor) == AI_SUCCESS)
        {
            tMaterialAsset->setEmissiveColor({tColor.r, tColor.g, tColor.b});
        }

        if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, tColor) == AI_SUCCESS)
        {
            tMaterialAsset->setAmbientColor({tColor.r, tColor.g, tColor.b});
        }
    }

    {
        float tValue;

        if (pMaterial->Get(AI_MATKEY_SHININESS, tValue) == AI_SUCCESS)
        {
            tMaterialAsset->setShininess(tValue);
        }

        if (pMaterial->Get(AI_MATKEY_OPACITY, tValue) == AI_SUCCESS)
        {
            tMaterialAsset->setOpacity(tValue);
        }
    }

    {
        loadTexture(aiTextureType_DIFFUSE, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_SPECULAR, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_AMBIENT, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_EMISSIVE, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_HEIGHT, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_NORMALS, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_SHININESS, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_OPACITY, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_DISPLACEMENT, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_LIGHTMAP, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_REFLECTION, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_BASE_COLOR, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_NORMAL_CAMERA, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_EMISSION_COLOR, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_METALNESS, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_DIFFUSE_ROUGHNESS, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_AMBIENT_OCCLUSION, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_SHEEN, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_CLEARCOAT, pMaterial, tMaterialAsset);
        loadTexture(aiTextureType_TRANSMISSION, pMaterial, tMaterialAsset);
    }
}

void ModelLoader::loadTexture(aiTextureType pTextureType, aiMaterial* pMaterial, MaterialAsset* pMaterialAsset)
{
    unsigned int tTextureCount = pMaterial->GetTextureCount(pTextureType);

    if (tTextureCount == 0)
    {
        return;
    }

    if (tTextureCount > 1)
    {
        spdlog::warn("Texture count more than 0");
    }

    aiString tFilePath;

    if (pMaterial->Get(AI_MATKEY_TEXTURE(pTextureType, 0), tFilePath) != AI_SUCCESS)
    {
        spdlog::warn("Texture is not found.");
        return;
    }

    if (tFilePath.length == 0)
    {
        spdlog::warn("Texture path lengh cannot be 0.");
        return;
    }

    std::string tTexturePath = tFilePath.C_Str();

    TextureAsset* tTexture = nullptr;

    for (auto& tElement : mModelAsset->mTextures)
    {
        if (!tElement)
        {
            continue;
        }

        TextureAsset* tTextureAsset = dynamic_cast<TextureAsset*>(tElement);

        if (!tTextureAsset)
        {
            continue;
        }

        if (tTextureAsset->getPath() == tTexturePath)
        {
            tTexture = tTextureAsset;
            break;
        }
    }

    if (!tTexture)
    {
        // Load
        const aiTexture* tTextureObject = mSceneObject->GetEmbeddedTexture(tTexturePath.c_str());

        Image* tImage = nullptr;

        if (!tTextureObject)
        {
            tImage = createImageFromFile(tTexturePath);
        }
        else
        {
            tImage = createImageFromEmbedded(tTextureObject);
        }

        if (tImage)
        {
            tTexture = new TextureAsset(tTexturePath, tImage);
        }
        else
        {
            spdlog::error("Image object is not created.");
            return;
        }
    }

    switch (pTextureType)
    {
        case aiTextureType_DIFFUSE:
        {
            pMaterialAsset->setDiffuseTexture(tTexture);
            break;
        }
        case aiTextureType_SPECULAR:
        {
            pMaterialAsset->setSpecularTexture(tTexture);
            break;
        }
        case aiTextureType_AMBIENT:
        {
            pMaterialAsset->setAmbientTexture(tTexture);
            break;
        }
        case aiTextureType_EMISSIVE:
        {
            pMaterialAsset->setEmissiveTexture(tTexture);
            break;
        }
        case aiTextureType_HEIGHT:
        {
            pMaterialAsset->setHeightTexture(tTexture);
            break;
        }
        case aiTextureType_NORMALS:
        {
            pMaterialAsset->setNormalsTexture(tTexture);
            break;
        }
        case aiTextureType_SHININESS:
        {
            pMaterialAsset->setShininessTexture(tTexture);
            break;
        }
        case aiTextureType_OPACITY:
        {
            pMaterialAsset->setOpacityTexture(tTexture);
            break;
        }
        case aiTextureType_DISPLACEMENT:
        {
            pMaterialAsset->setDisplacementTexture(tTexture);
            break;
        }
        case aiTextureType_LIGHTMAP:
        {
            pMaterialAsset->setLightmapTexture(tTexture);
            break;
        }
        case aiTextureType_REFLECTION:
        {
            pMaterialAsset->setReflectionTexture(tTexture);
            break;
        }
        case aiTextureType_BASE_COLOR:
        {
            pMaterialAsset->setBaseColorTexture(tTexture);
            break;
        }
        case aiTextureType_NORMAL_CAMERA:
        {
            pMaterialAsset->setNormalCameraTexture(tTexture);
            break;
        }
        case aiTextureType_EMISSION_COLOR:
        {
            pMaterialAsset->setEmissionColorTexture(tTexture);
            break;
        }
        case aiTextureType_METALNESS:
        {
            pMaterialAsset->setMetalnessTexture(tTexture);
            break;
        }
        case aiTextureType_DIFFUSE_ROUGHNESS:
        {
            pMaterialAsset->setDiffuseRoughnessTexture(tTexture);
            break;
        }
        case aiTextureType_AMBIENT_OCCLUSION:
        {
            pMaterialAsset->setAmbientOcclusionTexture(tTexture);
            break;
        }
        case aiTextureType_SHEEN:
        {
            pMaterialAsset->setSheenTexture(tTexture);
            break;
        }
        case aiTextureType_CLEARCOAT:
        {
            pMaterialAsset->setClearcoatTexture(tTexture);
            break;
        }
        case aiTextureType_TRANSMISSION:
        {
            pMaterialAsset->setTransmissionTexture(tTexture);
            break;
        }
        default:
        {
            spdlog::error("Undefined texture type. {0}", (int)pTextureType);
        }
    }
}

Image* ModelLoader::createImageFromFile(const std::string& pPath)
{
    std::string tImagePath = findImagePath(pPath);

    if (tImagePath.size() == 0)
    {
        spdlog::info("Image is not found: {0}", pPath);
        return nullptr;
    }
    // else
    // {
    //     spdlog::info("Image is found: {0}", tImagePath);
    // }

    Image* tImage = new Image();

    tImage->data = stbi_load(tImagePath.c_str(),
                             &tImage->width,
                             &tImage->height,
                             &tImage->channels,
                             0);

    return tImage;
}

Image* ModelLoader::createImageFromEmbedded(const aiTexture* pTexture)
{
    if (!pTexture)
    {
        return nullptr;
    }

    Image* tImage = new Image();

    if (pTexture->mHeight == 0)
    {
        tImage->data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(pTexture->pcData),
                                             pTexture->mWidth,
                                             &tImage->width,
                                             &tImage->height,
                                             &tImage->channels,
                                             0);
    }
    else
    {
        tImage->data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(pTexture->pcData),
                                             pTexture->mWidth * pTexture->mHeight,
                                             &tImage->width,
                                             &tImage->height,
                                             &tImage->channels,
                                             0);
    }

    return tImage;
}

std::string ModelLoader::findImagePath(const std::string& pPath)
{
    std::string tFileName = FileManager::getFileNameFromPath(pPath);

    for (auto& tLocation : mPossibleTextureLocations)
    {
        std::string tPath = FileManager::createPath(tLocation, tFileName);

        if (FileManager::isFileExist(tPath))
        {
            return tPath;
        }
    }

    return "";
}

void ModelLoader::createPossibleTextureLocations()
{
    std::filesystem::path tSourceDirectory = FileManager::getDirectoryFromPath(mPath);

    mPossibleTextureLocations.push_back(tSourceDirectory.string());
    mPossibleTextureLocations.push_back(FileManager::createPath(tSourceDirectory, "textures"));

    if (FileManager::hasParentPath(tSourceDirectory))
    {
        std::string tParentDirectory = FileManager::getParentDirectory(tSourceDirectory);
        mPossibleTextureLocations.push_back(FileManager::createPath(tParentDirectory, "textures"));
        mPossibleTextureLocations.push_back(tParentDirectory);
    }
}

DataContainer* ModelLoader::createVertexInformation(aiMesh* pMesh, MeshAsset* pMeshAsset)
{
    unsigned int tVerticeCount = pMesh->mNumVertices;

    DataContainer* tVertexInformation = new DataContainer();

    std::vector<glm::vec3>& tPositionsBuffer = tVertexInformation->getBuffer<glm::vec3>("positions");
    tPositionsBuffer.reserve(tVerticeCount);

    std::vector<glm::vec3>& tNormalsBuffer = tVertexInformation->getBuffer<glm::vec3>("normals");
    tNormalsBuffer.reserve(tVerticeCount);

    auto* tVertices = pMesh->mVertices;
    auto* tNormals = pMesh->mNormals;
    auto* tTextureCoordinates = pMesh->mTextureCoords[0];

    if (tTextureCoordinates)
    {
        std::vector<glm::vec2>& tTextureCoordinatesBuffer = tVertexInformation->getBuffer<glm::vec2>("textureCoordinates");
        tTextureCoordinatesBuffer.reserve(tVerticeCount);

        for (unsigned int i = 0; i < tVerticeCount; i++)
        {
            auto& tPosition = tVertices[i];
            tPositionsBuffer.emplace_back(glm::vec3(tPosition.x, tPosition.y, tPosition.z));

            auto& tNormal = tNormals[i];
            tNormalsBuffer.emplace_back(glm::vec3(tNormal.x, tNormal.y, tNormal.z));

            auto& tTextureCoordinate = tTextureCoordinates[i];
            tTextureCoordinatesBuffer.emplace_back(glm::vec2(tTextureCoordinate.x, tTextureCoordinate.y));
        }
    }
    else
    {
        std::vector<glm::vec2>& tTextureCoordinatesBuffer = tVertexInformation->getBuffer<glm::vec2>("textureCoordinates");
        tTextureCoordinatesBuffer.resize(tVerticeCount, glm::vec2(0.0f, 0.0f));

        for (unsigned int i = 0; i < tVerticeCount; i++)
        {
            auto& tPosition = tVertices[i];
            tPositionsBuffer.emplace_back(glm::vec3(tPosition.x, tPosition.y, tPosition.z));

            auto& tNormal = tNormals[i];
            tNormalsBuffer.emplace_back(glm::vec3(tNormal.x, tNormal.y, tNormal.z));
        }
    }

    unsigned int tFaceCount = pMesh->mNumFaces;

    std::vector<unsigned int>& tIndicesBuffer = tVertexInformation->getBuffer<unsigned int>("indices");
    tIndicesBuffer.reserve(tFaceCount * 3);

    for (unsigned int i = 0; i < tFaceCount; i++)
    {
        auto& tFace = pMesh->mFaces[i];

        if (tFace.mNumIndices != 3)
        {
            spdlog::error("A face has {0} indices.", tFace.mNumIndices);
            continue;
        }

        tIndicesBuffer.push_back(tFace.mIndices[0]);
        tIndicesBuffer.push_back(tFace.mIndices[1]);
        tIndicesBuffer.push_back(tFace.mIndices[2]);
    }

    bool tHasBones = pMesh->HasBones();

    if (tHasBones)
    {
        std::vector<glm::ivec4>& tBoneIDsBuffer = tVertexInformation->getBuffer<glm::ivec4>("boneIDs");
        tBoneIDsBuffer.resize(tVerticeCount, glm::ivec4(-1, -1, -1, -1));

        std::vector<glm::vec4>& tBoneWeightsBuffer = tVertexInformation->getBuffer<glm::vec4>("boneWeights");
        tBoneWeightsBuffer.resize(tVerticeCount, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

        pMeshAsset->setHasBones(true);

        aiBone** tBones = pMesh->mBones;

        if (tBones)
        {
            unsigned int tBoneCount = pMesh->mNumBones;

            for (unsigned int i = 0; i < tBoneCount; i++)
            {
                aiBone* tBone = tBones[i];
                std::string tBoneName = tBone->mName.C_Str();

                BoneAsset* tBoneAsset = nullptr;
                int tBoneId = -1;

                {
                    const std::vector<ABone*>& tCurrentBones = mModelAsset->mBones;
                    unsigned int tBoneCount = tCurrentBones.size();

                    for (unsigned int j = 0; j < tBoneCount; j++)
                    {
                        ABone* tTempBone = tCurrentBones[j];

                        if (!tTempBone)
                        {
                            continue;
                        }

                        if (tTempBone->getName() == tBoneName)
                        {
                            BoneAsset* tTempBoneAsset = dynamic_cast<BoneAsset*>(tTempBone);

                            if (!tTempBoneAsset)
                            {
                                throw "Temp bone asset is not found.";
                            }

                            tBoneAsset = tTempBoneAsset;
                            tBoneId = j;
                            break;
                        }
                    }
                }

                if (!tBoneAsset)
                {
                    // TODO: Create bone.
                }

                glm::ivec4& tVerticeBones = tBoneIDsBuffer[tBoneId];
                glm::vec4& tVerticeBoneWeights = tBoneWeightsBuffer[tBoneId];

                unsigned int tWeightCount = tBone->mNumWeights;
                aiVertexWeight* tWeights = tBone->mWeights;

                for (unsigned int j = 0; j < tWeightCount; j++)
                {
                    aiVertexWeight& tVertexWeights = tWeights[j];
                    float tVerticeWeight = tVertexWeights.mWeight;

                    if (tVerticeWeight == 0.0f)
                    {
                        continue;
                    }

                    for (unsigned int k = 0; k < 4; k++)     // Maximum 4 bone is supported.
                    {
                        if (tVerticeBones[k] == tBoneId)
                        {
                            throw "A bone affects a vertice only once.";
                        }

                        if (tVerticeBones[k] == -1)
                        {
                            tVerticeBones[k] = tBoneId;
                            tVerticeBoneWeights[k] = tVerticeWeight;
                            break;
                        }
                    }
                }
            }

            unsigned int tVerticeCount = tBoneIDsBuffer.size();

            for (unsigned int i = 0; i < tVerticeCount; i++)
            {
                float tTotalWeight = 0.0f;
                unsigned int tBoneCount = 0;

                glm::ivec4& tBoneIds = tBoneIDsBuffer[i];
                glm::vec4& tBoneWeights = tBoneWeightsBuffer[i];

                for (unsigned int j = 0; j < 4; j++)
                {
                    if (tBoneIds[j] == -1)
                    {
                        break;
                    }

                    tTotalWeight += tBoneWeights[j];
                    tBoneCount++;
                }

                if (tBoneCount > 0)
                {
                    if (tTotalWeight != 0.0f && std::abs(tTotalWeight - 1.0f) <= 1e-3)
                    {
                        for (unsigned int j = 0; j < tBoneCount; j++)
                        {
                            tBoneWeights[j] /= tTotalWeight;
                        }
                    }
                }
            }
        }
    }

    return tVertexInformation;
}
