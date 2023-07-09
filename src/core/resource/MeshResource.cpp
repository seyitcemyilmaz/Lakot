#include "MeshResource.h"

#include "platform/Platform.h"

MeshResource::MeshResource(std::string pName, unsigned int pMaterialIndex, NodeResource* pConnectedNode) {
	mName = pName;
	mConnectedNode = pConnectedNode;
	mMaterialIndex = pMaterialIndex;

	mVAO = 0;
	mVBO = 0;
	mIBO = 0;

	mIndexCount = 0;

	mHasBone = false;
}

std::string MeshResource::getName() {
	return mName;
}

unsigned int MeshResource::getMaterialIndex() {
	return mMaterialIndex;
}

void MeshResource::setHasBone(bool pHasBone) {
	mHasBone = pHasBone;
}

void MeshResource::createBuffers(std::vector<Vertex>& tVertexList, std::vector<unsigned int>& tIndexList) {
	mIndexCount = tIndexList.size();

	glGenVertexArrays(1, &mVAO);
	glGenBuffers(1, &mVBO);
	glGenBuffers(1, &mIBO);

	glBindVertexArray(mVAO);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tIndexList.size() * sizeof(unsigned int), tIndexList.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, tVertexList.size() * sizeof(Vertex), tVertexList.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, textureCoordinates));
	glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIds));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));

	glBindVertexArray(0);
}

void MeshResource::draw() {
	glBindVertexArray(mVAO);
	glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, nullptr);
}
