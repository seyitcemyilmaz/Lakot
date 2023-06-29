#include "noderesource.h"

NodeResource::NodeResource(std::string pName, NodeResource* pParentNode, glm::mat4& pTransformationMatrix) {
	mName = pName;
	mParentNode = pParentNode;
	mTransformationMatrix = pTransformationMatrix;
}

std::string NodeResource::getName() {
	return mName;
}

void NodeResource::addChildNode(NodeResource* pChildNode) {
	mChildNodes.push_back(pChildNode);
}
