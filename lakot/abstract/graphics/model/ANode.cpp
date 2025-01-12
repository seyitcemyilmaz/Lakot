#include "ANode.h"

using namespace lakot;

ANode::~ANode()
{

}

ANode::ANode(ANode* pParentNode, const glm::mat4& pTransformation)
    : Object()
    , mParentNode(pParentNode)
    , mTransformation(pTransformation)
{

}

ANode* ANode::getParentNode() const
{
    return mParentNode;
}

glm::mat4& ANode::getTransformation()
{
    return mTransformation;
}

const std::vector<ANode*>& ANode::getChildNodes() const
{
    return mChildNodes;
}

const std::vector<AMesh*>& ANode::getChildMeshes() const
{
    return mChildMeshes;
}
