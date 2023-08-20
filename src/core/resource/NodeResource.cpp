#include "NodeResource.h"

NodeResource::NodeResource(const std::string& pName, NodeResource* pParentNode, glm::mat4& pTransformationMatrix)
	: mName(pName)
	, mParentNode(pParentNode)
	, mTransformationMatrix(pTransformationMatrix) { }

const std::string& NodeResource::getName() const {
	return mName;
}

void NodeResource::addChildNode(NodeResource* pChildNode) {
	mChildNodes.push_back(pChildNode);
}

const std::vector<NodeResource*>& NodeResource::getChildNodes() const {
	return mChildNodes;
}

const std::vector<MeshResource*>& NodeResource::getChildMeshes() const {
	return mChildMeshes;
}

bool NodeResource::hasChildMesh() const {
	return !mChildMeshes.empty();
}

void NodeResource::addChildMesh(MeshResource* pChildMesh) {
	mChildMeshes.push_back(pChildMesh);
}
