#include "MeshResource.h"

#include "ModelResource.h"

MeshResource::MeshResource(const std::string& pName,
						   ModelResource* pModelResource,
						   const std::vector<Vertex>& pVertexList,
						   const std::vector<unsigned int>& pIndexList,
						   unsigned int pMaterialIndex)
	: mName(pName)
	, mModelResource(pModelResource)
	, mVertexList(pVertexList)
	, mIndexList(pIndexList)
	, mMaterialIndex(pMaterialIndex) { }

const std::string& MeshResource::getName() const {
	return mName;
}

unsigned int MeshResource::getMaterialIndex() const {
	return mMaterialIndex;
}

void MeshResource::setHasBone(bool pHasBone) {
	mHasBone = pHasBone;
}

void MeshResource::setConnectedNode(NodeResource* pConnectedNode) {
	mConnectedNode = pConnectedNode;
}

unsigned int MeshResource::getVAO() {
	return mVAO;
}

unsigned int MeshResource::getIndiceCount() {
	return mIndiceCount;
}
