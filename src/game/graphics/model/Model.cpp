#include "Model.h"

#include "../../asset/AssetManager.h"

#include <glad/glad.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

using namespace lakot;

Model::Model()
    : Renderable()
{
    mRenderableType = RenderableType::eModel;
}

Model::~Model()
{
    deinitialize();
}

std::unique_ptr<Model> Model::load(const std::string& pPath, AssetManager& pAssets, std::string& pError)
{
    Assimp::Importer tImporter;

    // PreTransformVertices bakes node transforms into the vertices, so the
    // file's scene graph never has to be walked at draw time.
    const aiScene* tScene = tImporter.ReadFile(pPath,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices | aiProcess_JoinIdenticalVertices);

    if (!tScene || !tScene->HasMeshes())
    {
        pError = "cannot read " + pPath + ": " + tImporter.GetErrorString();
        return nullptr;
    }

    std::unique_ptr<Model> tModel(new Model());

    std::vector<glm::vec3> tPositions;
    std::vector<glm::vec3> tNormals;
    std::vector<glm::vec2> tUvs;
    std::vector<unsigned int> tIndices;

    for (unsigned int tMeshIndex = 0; tMeshIndex < tScene->mNumMeshes; ++tMeshIndex)
    {
        const aiMesh* tMesh = tScene->mMeshes[tMeshIndex];
        unsigned int tBase = static_cast<unsigned int>(tPositions.size());

        for (unsigned int tVertex = 0; tVertex < tMesh->mNumVertices; ++tVertex)
        {
            const aiVector3D& tPosition = tMesh->mVertices[tVertex];
            tPositions.emplace_back(tPosition.x, tPosition.y, tPosition.z);

            const aiVector3D tNormal = tMesh->HasNormals() ? tMesh->mNormals[tVertex] : aiVector3D(0.0f, 1.0f, 0.0f);
            tNormals.emplace_back(tNormal.x, tNormal.y, tNormal.z);

            // Assimp has already flipped V to OpenGL's bottom-left origin;
            // the texture is loaded flipped to match (see below).
            const aiVector3D tUv = tMesh->HasTextureCoords(0) ? tMesh->mTextureCoords[0][tVertex] : aiVector3D(0.0f);
            tUvs.emplace_back(tUv.x, tUv.y);
        }

        for (unsigned int tFace = 0; tFace < tMesh->mNumFaces; ++tFace)
        {
            const aiFace& tFaceData = tMesh->mFaces[tFace];

            for (unsigned int tCorner = 0; tCorner < tFaceData.mNumIndices; ++tCorner)
            {
                tIndices.push_back(tBase + tFaceData.mIndices[tCorner]);
            }
        }
    }

    tModel->mTexture = pAssets.getModelTexture(*tScene, pPath, pError);

    if (!tModel->mTexture)
    {
        return nullptr;
    }

    tModel->mIndexCount = static_cast<unsigned int>(tIndices.size());

    tModel->mVertexInformation.set("positions", tPositions);
    tModel->mVertexInformation.set("normals", tNormals);
    tModel->mVertexInformation.set("uvs", tUvs);
    tModel->mVertexInformation.set("indices", tIndices);
    tModel->mVertexInformation.set<glm::vec3>("instanceOffsets", {});
    tModel->mVertexInformation.set<glm::vec3>("instanceParams", {});

    return tModel;
}

void Model::initialize()
{
    if (mIsInitialized)
    {
        return;
    }

    // Layout order is the attribute locations the world_model shader reads.
    createIndexBuffer("indices");
    createStaticBuffer("positions", VertexBufferObjectDataType::eVec3);      // 0
    createStaticBuffer("normals", VertexBufferObjectDataType::eVec3);        // 1
    createStaticBuffer("uvs", VertexBufferObjectDataType::eVec2);            // 2
    createInstancedBuffer("instanceOffsets", VertexBufferObjectDataType::eVec3);   // 3
    createInstancedBuffer("instanceParams", VertexBufferObjectDataType::eVec3);    // 4

    mVertexArrayObject.initialize();

    syncIndexData("indices");
    syncBufferData<glm::vec3>(0, "positions");
    syncBufferData<glm::vec3>(1, "normals");
    syncBufferData<glm::vec2>(2, "uvs");
    syncBufferData<glm::vec3>(3, "instanceOffsets");
    syncBufferData<glm::vec3>(4, "instanceParams");


    mIsNeedUpdate = false;
    mIsInitialized = true;
}

void Model::deinitialize()
{
    if (!mIsInitialized)
    {
        return;
    }


    mVertexArrayObject.deinitialize();
    mIsInitialized = false;
}

void Model::setInstances(const std::vector<glm::vec3>& pOffsets, const std::vector<glm::vec3>& pYawScaleStretches)
{
    mVertexInformation.set<glm::vec3>("instanceOffsets", pOffsets);
    mVertexInformation.set<glm::vec3>("instanceParams", pYawScaleStretches);
    mInstanceCount = static_cast<unsigned int>(pOffsets.size());

    if (mIsInitialized)
    {
        syncBufferData<glm::vec3>(3, "instanceOffsets");
        syncBufferData<glm::vec3>(4, "instanceParams");
    }
}

void Model::draw(unsigned int pTextureUnit) const
{
    if (!mIsInitialized || mInstanceCount == 0)
    {
        return;
    }

    mTexture->bind(pTextureUnit);

    mVertexArrayObject.bind();
    glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(mIndexCount), GL_UNSIGNED_INT, nullptr,
                            static_cast<GLsizei>(mInstanceCount));
}
