#include "Node.h"

Node::Node(NodeResource* pNodeResource, Node* pParentNode, const glm::mat4& pTransformationMatrix)
    : mNodeResource(pNodeResource)
    , mParentNode(pParentNode)
    , mTransformationMatrix(pTransformationMatrix)
    , mBone(nullptr)
{

}

const std::string& Node::getName() const
{
    return mNodeResource->getName();
}

const INode* Node::getParentNode() const
{
    return mParentNode;
}

const glm::mat4& Node::getTransformationMatrix() const
{
    return mTransformationMatrix;
}

void Node::setTransformationMatrix(const glm::mat4& pTransformationMatrix)
{
    mTransformationMatrix = pTransformationMatrix;
}

const std::vector<INode*>& Node::getChildNodes() const
{
    return mChildNodes;
}

unsigned int Node::getChildNodeCount() const
{
    return static_cast<unsigned int>(mChildNodes.size());
}

IBone* Node::getBone() const
{
    return mBone;
}

void Node::setBone(IBone* pBone)
{
    mBone = pBone;
}

NodeResource* Node::getNodeResource()
{
    return mNodeResource;
}

void Node::addChildNode(Node* pNode)
{
    mChildNodes.push_back(pNode);
}

void Node::addChildMesh(Mesh* pMesh)
{
    mChildMeshes.push_back(pMesh);
}


