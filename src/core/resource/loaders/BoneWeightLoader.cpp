#include "BoneWeightLoader.h"

BoneWeightLoader::BoneWeightLoader(aiBone* pBone, std::vector<Vertex>& pVertexList, unsigned int pBoneId)
    : mBone(pBone)
    , mVertexList(pVertexList)
    , mBoneId(pBoneId)
{

}

void BoneWeightLoader::load()
{
    unsigned int tWeightCount = mBone->mNumWeights;
    const aiVertexWeight* tWeights = mBone->mWeights;

    for (unsigned int i = 0; i < tWeightCount; i++)
    {
        float tVertexWeight = tWeights[i].mWeight;
        unsigned int tVertexIndex = tWeights[i].mVertexId;
        Vertex* tVertex = &mVertexList[tVertexIndex];

        setVertexBoneWeight(tVertex, mBoneId, tVertexWeight);
    }
}

void BoneWeightLoader::setVertexBoneWeight(Vertex* pVertex, unsigned int pBoneId, float pWeight)
{
    if (pWeight == 0.0f)
    {
        return;
    }

    for (int i = 0; i < LAKOT_VERTEX_MAX_BONE_COUNT; i++)
    {
        if (pVertex->boneIds[i] == pBoneId)
        {
            break;
        }

        if (pVertex->boneIds[i] == -1)
        {
            pVertex->boneIds[i] = pBoneId;
            pVertex->boneWeights[i] = pWeight;
            break;
        }
    }
}
