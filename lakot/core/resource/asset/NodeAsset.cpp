#include "NodeAsset.h"

using namespace lakot;

NodeAsset::~NodeAsset()
{

}

NodeAsset::NodeAsset(const std::string& pName, const glm::mat4& pTransform, NodeAsset* pParentNode)
    : AAsset()
    , mName(pName)
    , mTransform(pTransform)
    , mParentNode(pParentNode)
{
    mType = AssetType::eNode;
}

void NodeAsset::initialize()
{

}

void NodeAsset::deinitialize()
{

}

const std::string& NodeAsset::getName() const
{
    return mName;
}

const glm::mat4& NodeAsset::getTransform() const
{
    return mTransform;
}

NodeAsset* NodeAsset::getParentNode() const
{
    return mParentNode;
}

const std::vector<NodeAsset*>& NodeAsset::getChildNodes() const
{
    return mChildNodes;
}

const std::vector<MeshAsset*>& NodeAsset::getChildMeshes() const
{
    return mChildMeshes;
}

void NodeAsset::addChildNode(NodeAsset* pNodeAsset)
{
    mChildNodes.push_back(pNodeAsset);
}

void NodeAsset::addChildMesh(MeshAsset* pMeshAsset)
{
    mChildMeshes.push_back(pMeshAsset);
}
