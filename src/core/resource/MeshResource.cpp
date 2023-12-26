#include "MeshResource.h"

MeshResource::MeshResource(const std::string& pName,
                           const std::vector<Vertex>& pVertexList,
                           const std::vector<unsigned int>& pIndexList,
                           unsigned int pMaterialIndex)
    : mName(pName)
    , mVertexList(pVertexList)
    , mIndexList(pIndexList)
    , mConnectedNode(nullptr)
    , mVAO(0)
    , mVBO(0)
    , mIBO(0)
    , mIndiceCount(0)
    , mVertexCount(0)
    , mMaterialIndex(pMaterialIndex)
    , mHasBone(false)
{
    mIndiceCount = static_cast<unsigned int>(mIndexList.size());
    mVertexCount = static_cast<unsigned int>(mVertexList.size());
}

const std::string& MeshResource::getName() const
{
    return mName;
}

unsigned int MeshResource::getMaterialIndex() const
{
    return mMaterialIndex;
}

void MeshResource::setHasBone(bool pHasBone)
{
    mHasBone = pHasBone;
}

void MeshResource::setConnectedNode(NodeResource* pConnectedNode)
{
    mConnectedNode = pConnectedNode;
}

unsigned int MeshResource::getVAO()
{
    return mVAO;
}

unsigned int MeshResource::getIndiceCount()
{
    return mIndiceCount;
}

unsigned int MeshResource::getVertexCount()
{
    return mVertexCount;
}

bool MeshResource::getHasBone() const {
    return mHasBone;
}
