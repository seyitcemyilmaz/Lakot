#include "Node.h"

Node::Node(NodeResource* pNodeResource, Node* pParentNode)
	: mNodeResource(pNodeResource)
	, mParentNode(pParentNode) { }

const std::string& Node::getName() {
	return mNodeResource->getName();
}

void Node::addChildNode(Node* pNode) {
	mChildNodes.push_back(pNode);
}

void Node::addChildMesh(Mesh* pMesh) {
	mChildMeshes.push_back(pMesh);
}
