#include "SkinnedModel.h"

#include "../../asset/AssetManager.h"

#include <algorithm>
#include <limits>
#include <unordered_map>

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

using namespace lakot;

namespace
{
    glm::mat4 toGlm(const aiMatrix4x4& pMatrix)
    {
        return glm::transpose(glm::make_mat4(&pMatrix.a1));
    }

    JointPose toPose(const aiMatrix4x4& pMatrix)
    {
        aiVector3D tScale;
        aiQuaternion tRotation;
        aiVector3D tTranslation;
        pMatrix.Decompose(tScale, tRotation, tTranslation);

        JointPose tPose;
        tPose.translation = glm::vec3(tTranslation.x, tTranslation.y, tTranslation.z);
        tPose.rotation = glm::quat(tRotation.w, tRotation.x, tRotation.y, tRotation.z);
        tPose.scale = glm::vec3(tScale.x, tScale.y, tScale.z);
        return tPose;
    }

    void addJoints(const aiNode* pNode, int pParent, Skeleton& pSkeleton, std::unordered_map<unsigned int, int>& pMeshJoints)
    {
        int tJoint = pSkeleton.addJoint(pNode->mName.C_Str(), pParent, toPose(pNode->mTransformation));

        for (unsigned int tIndex = 0; tIndex < pNode->mNumMeshes; ++tIndex)
        {
            pMeshJoints[pNode->mMeshes[tIndex]] = tJoint;
        }

        for (unsigned int tIndex = 0; tIndex < pNode->mNumChildren; ++tIndex)
        {
            addJoints(pNode->mChildren[tIndex], tJoint, pSkeleton, pMeshJoints);
        }
    }

    void addInfluence(glm::ivec4& pJoints, glm::vec4& pWeights, int pJoint, float pWeight)
    {
        for (int tSlot = 0; tSlot < 4; ++tSlot)
        {
            if (pWeights[tSlot] == 0.0f)
            {
                pJoints[tSlot] = pJoint;
                pWeights[tSlot] = pWeight;
                return;
            }
        }
    }

    std::vector<AnimationClip::Track> readTracks(const aiAnimation& pAnimation, const Skeleton& pSkeleton, float pTicksPerSecond)
    {
        std::vector<AnimationClip::Track> tTracks;

        for (unsigned int tChannelIndex = 0; tChannelIndex < pAnimation.mNumChannels; ++tChannelIndex)
        {
            const aiNodeAnim* tChannel = pAnimation.mChannels[tChannelIndex];

            AnimationClip::Track tTrack;
            tTrack.joint = pSkeleton.findJoint(tChannel->mNodeName.C_Str());

            if (tTrack.joint < 0)
            {
                continue;
            }

            for (unsigned int tKey = 0; tKey < tChannel->mNumPositionKeys; ++tKey)
            {
                const aiVectorKey& tValue = tChannel->mPositionKeys[tKey];
                tTrack.translations.push_back({ static_cast<float>(tValue.mTime) / pTicksPerSecond,
                                                glm::vec3(tValue.mValue.x, tValue.mValue.y, tValue.mValue.z) });
            }

            for (unsigned int tKey = 0; tKey < tChannel->mNumRotationKeys; ++tKey)
            {
                const aiQuatKey& tValue = tChannel->mRotationKeys[tKey];
                tTrack.rotations.push_back({ static_cast<float>(tValue.mTime) / pTicksPerSecond,
                                             glm::quat(tValue.mValue.w, tValue.mValue.x, tValue.mValue.y, tValue.mValue.z) });
            }

            for (unsigned int tKey = 0; tKey < tChannel->mNumScalingKeys; ++tKey)
            {
                const aiVectorKey& tValue = tChannel->mScalingKeys[tKey];
                tTrack.scales.push_back({ static_cast<float>(tValue.mTime) / pTicksPerSecond,
                                          glm::vec3(tValue.mValue.x, tValue.mValue.y, tValue.mValue.z) });
            }

            tTracks.push_back(std::move(tTrack));
        }

        return tTracks;
    }
}

SkinnedModel::SkinnedModel()
    : Renderable()
{
    mRenderableType = RenderableType::eSkinnedModel;
}

SkinnedModel::~SkinnedModel()
{
    deinitialize();
}

std::unique_ptr<SkinnedModel> SkinnedModel::load(const std::string& pPath, AssetManager& pAssets, std::string& pError)
{
    Assimp::Importer tImporter;

    const aiScene* tScene = tImporter.ReadFile(pPath,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_LimitBoneWeights);

    if (!tScene || !tScene->mRootNode)
    {
        pError = "cannot read " + pPath + ": " + tImporter.GetErrorString();
        return nullptr;
    }

    std::unique_ptr<SkinnedModel> tModel(new SkinnedModel());

    std::unordered_map<unsigned int, int> tMeshJoints;
    addJoints(tScene->mRootNode, -1, tModel->mSkeleton, tMeshJoints);

    if (tModel->mSkeleton.getJointCount() > kMaxJoints)
    {
        pError = pPath + " has more than " + std::to_string(kMaxJoints) + " joints";
        return nullptr;
    }

    std::vector<glm::vec3> tPositions;
    std::vector<glm::vec3> tNormals;
    std::vector<glm::vec2> tUvs;
    std::vector<glm::ivec4> tJoints;
    std::vector<glm::vec4> tWeights;
    std::vector<unsigned int> tIndices;

    for (unsigned int tMeshIndex = 0; tMeshIndex < tScene->mNumMeshes; ++tMeshIndex)
    {
        const aiMesh* tMesh = tScene->mMeshes[tMeshIndex];
        size_t tBase = tPositions.size();

        for (unsigned int tVertex = 0; tVertex < tMesh->mNumVertices; ++tVertex)
        {
            const aiVector3D& tPosition = tMesh->mVertices[tVertex];
            tPositions.emplace_back(tPosition.x, tPosition.y, tPosition.z);

            const aiVector3D tNormal = tMesh->HasNormals() ? tMesh->mNormals[tVertex] : aiVector3D(0.0f, 1.0f, 0.0f);
            tNormals.emplace_back(tNormal.x, tNormal.y, tNormal.z);

            const aiVector3D tUv = tMesh->HasTextureCoords(0) ? tMesh->mTextureCoords[0][tVertex] : aiVector3D(0.0f);
            tUvs.emplace_back(tUv.x, tUv.y);
        }

        tJoints.resize(tPositions.size(), glm::ivec4(0));
        tWeights.resize(tPositions.size(), glm::vec4(0.0f));

        int tMeshJoint = tMeshJoints.count(tMeshIndex) != 0 ? tMeshJoints[tMeshIndex] : 0;

        if (tMesh->HasBones())
        {
            for (unsigned int tBoneIndex = 0; tBoneIndex < tMesh->mNumBones; ++tBoneIndex)
            {
                const aiBone* tBone = tMesh->mBones[tBoneIndex];
                int tJoint = tModel->mSkeleton.findJoint(tBone->mName.C_Str());

                if (tJoint < 0)
                {
                    continue;
                }

                tModel->mSkeleton.setInverseBind(tJoint, toGlm(tBone->mOffsetMatrix));

                for (unsigned int tWeightIndex = 0; tWeightIndex < tBone->mNumWeights; ++tWeightIndex)
                {
                    const aiVertexWeight& tWeight = tBone->mWeights[tWeightIndex];
                    size_t tVertex = tBase + tWeight.mVertexId;
                    addInfluence(tJoints[tVertex], tWeights[tVertex], tJoint, tWeight.mWeight);
                }
            }
        }

        for (size_t tVertex = tBase; tVertex < tPositions.size(); ++tVertex)
        {
            if (tWeights[tVertex] == glm::vec4(0.0f))
            {
                tJoints[tVertex] = glm::ivec4(tMeshJoint, 0, 0, 0);
                tWeights[tVertex] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            }
        }

        for (unsigned int tFace = 0; tFace < tMesh->mNumFaces; ++tFace)
        {
            const aiFace& tFaceData = tMesh->mFaces[tFace];

            for (unsigned int tCorner = 0; tCorner < tFaceData.mNumIndices; ++tCorner)
            {
                tIndices.push_back(static_cast<unsigned int>(tBase) + tFaceData.mIndices[tCorner]);
            }
        }
    }

    for (glm::vec4& tWeight : tWeights)
    {
        float tSum = tWeight.x + tWeight.y + tWeight.z + tWeight.w;

        if (tSum > 0.0f)
        {
            tWeight /= tSum;
        }
    }

    std::vector<glm::mat4> tBindGlobals;
    tModel->mSkeleton.computePalette(tModel->mSkeleton.getBindPose(), tModel->mBindPalette, tBindGlobals);

    const std::vector<glm::mat4>& tBindPalette = tModel->mBindPalette;

    tModel->mBounds.min = glm::vec3(std::numeric_limits<float>::max());
    tModel->mBounds.max = glm::vec3(std::numeric_limits<float>::lowest());
    tModel->mJointBounds.assign(tModel->mSkeleton.getJointCount(), std::nullopt);

    for (size_t tVertex = 0; tVertex < tPositions.size(); ++tVertex)
    {
        glm::mat4 tSkin(0.0f);

        for (int tSlot = 0; tSlot < 4; ++tSlot)
        {
            tSkin += tBindPalette[static_cast<size_t>(tJoints[tVertex][tSlot])] * tWeights[tVertex][tSlot];
        }

        glm::vec3 tPosition = glm::vec3(tSkin * glm::vec4(tPositions[tVertex], 1.0f));
        tModel->mBounds.min = glm::min(tModel->mBounds.min, tPosition);
        tModel->mBounds.max = glm::max(tModel->mBounds.max, tPosition);

        int tDominantSlot = 0;

        for (int tSlot = 1; tSlot < 4; ++tSlot)
        {
            if (tWeights[tVertex][tSlot] > tWeights[tVertex][tDominantSlot])
            {
                tDominantSlot = tSlot;
            }
        }

        int tDominantJoint = tJoints[tVertex][tDominantSlot];
        glm::vec3 tLocal = tPosition;
        std::optional<Bounds>& tJointBounds = tModel->mJointBounds[static_cast<size_t>(tDominantJoint)];

        if (!tJointBounds)
        {
            tJointBounds = Bounds{ tLocal, tLocal };
        }

        tJointBounds->min = glm::min(tJointBounds->min, tLocal);
        tJointBounds->max = glm::max(tJointBounds->max, tLocal);
    }

    for (unsigned int tAnimationIndex = 0; tAnimationIndex < tScene->mNumAnimations; ++tAnimationIndex)
    {
        const aiAnimation* tAnimation = tScene->mAnimations[tAnimationIndex];
        float tTicksPerSecond = tAnimation->mTicksPerSecond > 0.0 ? static_cast<float>(tAnimation->mTicksPerSecond) : 25.0f;

        tModel->mClips.emplace_back(tAnimation->mName.C_Str(),
                                    static_cast<float>(tAnimation->mDuration) / tTicksPerSecond,
                                    readTracks(*tAnimation, tModel->mSkeleton, tTicksPerSecond));
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
    tModel->mVertexInformation.set("joints", tJoints);
    tModel->mVertexInformation.set("weights", tWeights);
    tModel->mVertexInformation.set("indices", tIndices);

    return tModel;
}

void SkinnedModel::initialize()
{
    if (mIsInitialized)
    {
        return;
    }

    createIndexBuffer("indices");
    createStaticBuffer("positions", VertexBufferObjectDataType::eVec3);
    createStaticBuffer("normals", VertexBufferObjectDataType::eVec3);
    createStaticBuffer("uvs", VertexBufferObjectDataType::eVec2);
    createStaticBuffer("joints", VertexBufferObjectDataType::eIVec4);
    createStaticBuffer("weights", VertexBufferObjectDataType::eVec4);

    mVertexArrayObject.initialize();

    syncIndexData("indices");
    syncBufferData<glm::vec3>(0, "positions");
    syncBufferData<glm::vec3>(1, "normals");
    syncBufferData<glm::vec2>(2, "uvs");
    syncBufferData<glm::ivec4>(3, "joints");
    syncBufferData<glm::vec4>(4, "weights");


    mIsNeedUpdate = false;
    mIsInitialized = true;
}

void SkinnedModel::deinitialize()
{
    if (!mIsInitialized)
    {
        return;
    }

    mVertexArrayObject.deinitialize();
    mIsInitialized = false;
}

const Skeleton& SkinnedModel::getSkeleton() const
{
    return mSkeleton;
}

const AnimationClip* SkinnedModel::findClip(const std::string& pName) const
{
    for (const AnimationClip& tClip : mClips)
    {
        if (tClip.getName() == pName)
        {
            return &tClip;
        }
    }

    return nullptr;
}

const SkinnedModel::Bounds& SkinnedModel::getBounds() const
{
    return mBounds;
}

const std::vector<AnimationClip>& SkinnedModel::getClips() const
{
    return mClips;
}

float SkinnedModel::getHeight() const
{
    return std::max(mBounds.max.y - mBounds.min.y, 0.001f);
}

const std::optional<SkinnedModel::Bounds>& SkinnedModel::getJointBounds(int pJoint) const
{
    return mJointBounds[static_cast<size_t>(pJoint)];
}

const std::vector<glm::mat4>& SkinnedModel::getBindPalette() const
{
    return mBindPalette;
}

void SkinnedModel::draw(unsigned int pTextureUnit) const
{
    if (!mIsInitialized)
    {
        return;
    }

    mTexture->bind(pTextureUnit);

    mVertexArrayObject.bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mIndexCount), GL_UNSIGNED_INT, nullptr);
}
