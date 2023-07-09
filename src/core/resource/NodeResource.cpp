#include "NodeResource.h"

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

std::vector<NodeResource*> NodeResource::getChildNodes() {
	return mChildNodes;
}

void NodeResource::addChildMesh(MeshResource* pChildMesh) {
	mChildMeshes.push_back(pChildMesh);
}

std::vector<MeshResource*> NodeResource::getChildMeshes() {
	return mChildMeshes;
}

bool NodeResource::hasChildMesh() {
	return mChildMeshes.size() > 0;
}
