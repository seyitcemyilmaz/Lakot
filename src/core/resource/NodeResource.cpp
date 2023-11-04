#include "NodeResource.h"

NodeResource::NodeResource(const std::string& pName, NodeResource* pParentNode, glm::mat4& pTransformationMatrix)
	: mName(pName)
	, mParentNode(pParentNode)
	, mTransformationMatrix(pTransformationMatrix) { }

const std::string& NodeResource::getName() const {
	return mName;
}

const std::vector<NodeResource*>& NodeResource::getChildNodes() const {
	return mChildNodes;
}

const std::vector<MeshResource*>& NodeResource::getChildMeshes() const {
	return mChildMeshes;
}

BoneResource* NodeResource::getBone() {
	return mBoneResource;
}

bool NodeResource::hasChildMesh() const {
	return !mChildMeshes.empty();
}

const glm::mat4& NodeResource::getTransformationMatrix() {
	return mTransformationMatrix;
}

void NodeResource::setBoneResource(BoneResource* pBoneResource) {
	mBoneResource = pBoneResource;
}

void NodeResource::addChildMesh(MeshResource* pChildMesh) {
	mChildMeshes.push_back(pChildMesh);
}

void NodeResource::addChildNode(NodeResource* pChildNode) {
	mChildNodes.push_back(pChildNode);
}
