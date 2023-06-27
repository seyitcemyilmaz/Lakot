#include "modelloader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include "textureloader.h"

ModelLoader::ModelLoader(std::string pModelPath) {
    mModelPath = Platform::getLakotModelsPath() + "/" + pModelPath;

    mFlags = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_FlipUVs | aiProcess_SplitByBoneCount;

    mModelResource = nullptr;
}

ModelResource* ModelLoader::loadModel() {
    Assimp::Importer tImporter;

    const aiScene* tScene = tImporter.ReadFile(mModelPath, mFlags);

    if (!tScene) {
        tImporter.FreeScene();
        delete tScene;
        return nullptr;
    }

    mModelResource = new ModelResource();

    //processNode(tScene->mRootNode, tScene, nullptr);

    // TODO: Will be continued.

    extractMaterials(tScene);

    return nullptr;
}

void ModelLoader::extractMaterials(const aiScene* pScene) {
    unsigned int tMaterialCount = pScene->mNumMaterials;

    aiString tTempFilePath;

    for (unsigned int i = 0; i < tMaterialCount; i++) {
        aiMaterial* tMaterial = pScene->mMaterials[i];

        // TODO: Add embedded texture support.

        tMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), tTempFilePath);

        std::string tFilePath = tTempFilePath.C_Str();

        if (!tFilePath.empty()) {
            if ('*' == tFilePath[0])  {
                std::cout << "Embedded !!!!!" << std::endl;
            }
            else {
                TextureLoader* tTextureLoader = new TextureLoader(mModelPath, tFilePath);

                if (!tTextureLoader->loadTexture()) {
                    std::cout << tFilePath << " is not found." << std::endl;
                }

                // TODO: Handle texture.
            }
        }
        else {
            std::cout << "There is no diffuse texture." << std::endl;
        }
    }
}

void ModelLoader::processNode(aiNode* pNode, const aiScene* pScene, NodeResource* pParentNodeResource) {
    std::string tNodeName = pNode->mName.C_Str();
    glm::mat4 tTransformationMatrix = assimpMatrixToGlmMatrix(pNode->mTransformation);

    NodeResource* tNodeResource = new NodeResource(tNodeName, pParentNodeResource, tTransformationMatrix);

    if (pParentNodeResource) {
        pParentNodeResource->addChildNode(tNodeResource);
    }
    else {
        mModelResource->setRootNode(tNodeResource);
    }

    unsigned int tMeshCount = pNode->mNumMeshes;

    for (unsigned int i = 0; i < tMeshCount; i++) {
        processMesh(pScene->mMeshes[pNode->mMeshes[i]], pScene, tNodeResource);
    }

    unsigned int tChildNodeCount = pNode->mNumChildren;

    for (unsigned int i = 0; i < tChildNodeCount; i++) {
        processNode(pNode->mChildren[i], pScene, tNodeResource);
    }
}

void ModelLoader::processMesh(aiMesh* pMesh, const aiScene* pScene, NodeResource* pConnectedNodeResource) {
    std::string tMeshName = pMesh->mName.C_Str();

    MeshResource* tMeshResource = new MeshResource(tMeshName, pConnectedNodeResource);

    std::vector<Vertex> tVertexList;
    createVertexList(pMesh, &tVertexList);

    std::vector<unsigned int> tIndexList;
    createIndexList(pMesh, &tIndexList);

    if (pMesh->HasBones()) {
        extractBones(pMesh, &tVertexList);
        normalizeVertexBoneWeights(&tVertexList);
        tMeshResource->setHasBone(true);
    }

    tMeshResource->createBuffers(&tVertexList, &tIndexList);

    mModelResource->addMeshResource(tMeshResource);
}

void ModelLoader::extractBones(aiMesh* pMesh, std::vector<Vertex>* pVertexList) {
    aiBone** tBones = pMesh->mBones;
    unsigned int tBoneCount = pMesh->mNumBones;

    for (unsigned int i = 0; i < tBoneCount; i++) {
        aiBone* tBone = tBones[i];
        std::string tBoneName = tBone->mName.C_Str();

        if (!mModelResource->mBoneMap.contains(tBoneName)) {
            BoneResource* tBoneResource = new BoneResource(tBoneName, assimpMatrixToGlmMatrix(tBone->mOffsetMatrix));
            mModelResource->mBoneMap[tBoneName] = tBoneResource;
            mModelResource->addBoneResource(tBoneResource);
        }

        extractVertexBoneWeights(tBone, pVertexList);
    }
}

void ModelLoader::extractVertexBoneWeights(aiBone* pBone, std::vector<Vertex>* pVertexList) {
    int tBoneId = mModelResource->getBoneId(pBone->mName.C_Str());

    unsigned int tWeightCount = pBone->mNumWeights;
    aiVertexWeight* tWeights = pBone->mWeights;

    for (unsigned int i = 0; i < tWeightCount; i++) {
        float tVertexWeight = tWeights[i].mWeight;
        unsigned int tVertexIndex = tWeights[i].mVertexId;
        Vertex* tVertex = &pVertexList->at(tVertexIndex);

        setVertexBoneWeight(tVertex, tBoneId, tVertexWeight);
    }
}

void ModelLoader::normalizeVertexBoneWeights(std::vector<Vertex>* pVertexList) {
    unsigned int tVertexCount = pVertexList->size();

    for (unsigned int i = 0; i < tVertexCount; i++) {
        Vertex* tVertex = &pVertexList->at(i);

        if (tVertex->boneIds[0] != -1) {
            unsigned int tVertexBoneCount = 0;
            float tTotalWeight = 0.0f;

            for (unsigned int j = 0; j < LAKOT_VERTEX_MAX_BONE_COUNT; j++) {
                if (tVertex->boneIds[j] == -1) {
                    break;
                }
                tTotalWeight += tVertex->boneWeights[j];
                tVertexBoneCount++;
            }

            if (std::abs(1.0f - tTotalWeight) > 0.01) {
                for (unsigned int j = 0; j < tVertexBoneCount; j++) {
                    tVertex->boneWeights[j] /= tTotalWeight;
                }
            }
        }
    }
}


void ModelLoader::setVertexBoneWeight(Vertex* pVertex, int pBoneId, float pWeight) {
    if (pWeight == 0.0f) {
        return;
    }

    for (int i = 0; i < LAKOT_VERTEX_MAX_BONE_COUNT; i++) {
        if (pVertex->boneIds[i] == pBoneId) {
            break;
        }

        if (pVertex->boneIds[i] == -1) {
            pVertex->boneIds[i] = pBoneId;
            pVertex->boneWeights[i] = pWeight;
            break;
        }
    }
}


void ModelLoader::createVertexList(aiMesh* pMesh, std::vector<Vertex>* pVertexList) {
    unsigned int tVertexCount = pMesh->mNumVertices;

    for (unsigned int i = 0; i < tVertexCount; i++) {
        Vertex tVertex {
            assimp3DVectorToGlmVector(pMesh->mVertices[i]),
            assimp3DVectorToGlmVector(pMesh->mNormals[i])
        };

        if (pMesh->mTextureCoords[0]) {
            tVertex.textureCoordinates = glm::vec2(pMesh->mTextureCoords[0]->x, pMesh->mTextureCoords[0]->y);
        }
        else {
            tVertex.textureCoordinates = glm::vec2(0.0f);
        }

        for (unsigned int j = 0; j < LAKOT_VERTEX_MAX_BONE_COUNT; j++) {
            tVertex.boneIds[j] = -1;
            tVertex.boneWeights[j] = 0.0f;
        }

        pVertexList->push_back(tVertex);
    }
}

void ModelLoader::createIndexList(aiMesh* pMesh, std::vector<unsigned int>* pIndexList) {
    unsigned int tPolygonCount = pMesh->mNumFaces;

    for (unsigned int i = 0; i < tPolygonCount; i++) {
        if (pMesh->mFaces[i].mNumIndices != 3) {
            throw "A polygon has not 3 indices";
        }

        pIndexList->push_back(pMesh->mFaces[i].mIndices[0]);
        pIndexList->push_back(pMesh->mFaces[i].mIndices[1]);
        pIndexList->push_back(pMesh->mFaces[i].mIndices[2]);
    }
}

glm::mat4 ModelLoader::assimpMatrixToGlmMatrix(aiMatrix4x4& pMatrix) {
    return glm::mat4 {
        pMatrix.a1, pMatrix.b1, pMatrix.c1, pMatrix.d1,
        pMatrix.a2, pMatrix.b2, pMatrix.c2, pMatrix.d2,
        pMatrix.a3, pMatrix.b3, pMatrix.c3, pMatrix.d3,
        pMatrix.a4, pMatrix.b4, pMatrix.c4, pMatrix.d4
    };
}

glm::vec3 ModelLoader::assimp3DVectorToGlmVector(aiVector3D& pVector) {
    return glm::vec3 {
        pVector.x, pVector.y, pVector.z
    };
}
