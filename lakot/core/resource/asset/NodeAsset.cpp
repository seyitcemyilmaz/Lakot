#include "NodeAsset.h"

using namespace lakot;

NodeAsset::~NodeAsset()
{

}

NodeAsset::NodeAsset(const std::string& pName, NodeAsset* pParentNode, const glm::mat4& pTransformation)
    : AAsset(AssetType::eNode)
    , ANode(pParentNode, pTransformation)
    , mName(pName)
{

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
