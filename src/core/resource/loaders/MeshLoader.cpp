#include "MeshLoader.h"

#include "helper/AssimpToGLMHelper.h"

MeshLoader::MeshLoader(aiMesh* pMeshObject) : mMeshObject(pMeshObject) { }

MeshResource* MeshLoader::loadMesh() {
	std::string tMeshName = mMeshObject->mName.C_Str();
	unsigned int tMaterialIndex = mMeshObject->mMaterialIndex;

	std::vector<Vertex> tVertexList = createVertexList();
	std::vector<unsigned int> tIndexList = createIndexList();

	return new MeshResource(tMeshName, tVertexList, tIndexList, tMaterialIndex);
}

std::vector<Vertex> MeshLoader::createVertexList() {
	std::vector<Vertex> tVertexList;
	unsigned int tVertexCount = mMeshObject->mNumVertices;

	for (unsigned int i = 0; i < tVertexCount; i++) {
		Vertex tVertex {
			AssimpToGLMHelper::toVec3(mMeshObject->mVertices[i]),
			AssimpToGLMHelper::toVec3(mMeshObject->mNormals[i])
		};

		if (mMeshObject->mTextureCoords[0]) {
			tVertex.textureCoordinates = glm::vec2(mMeshObject->mTextureCoords[0]->x, mMeshObject->mTextureCoords[0]->y);
		}
		else {
			tVertex.textureCoordinates = glm::vec2(0.0f);
		}

		for (unsigned int j = 0; j < LAKOT_VERTEX_MAX_BONE_COUNT; j++) {
			tVertex.boneIds[j] = -1;
			tVertex.boneWeights[j] = 0.0f;
		}

		tVertexList.push_back(tVertex);
	}

	return tVertexList;
}

std::vector<unsigned int> MeshLoader::createIndexList() {
	std::vector<unsigned int> tIndexList;
	unsigned int tPolygonCount = mMeshObject->mNumFaces;

	for (unsigned int i = 0; i < tPolygonCount; i++) {
		if (mMeshObject->mFaces[i].mNumIndices != 3) {
			throw "A polygon has not 3 indices";
		}

		tIndexList.push_back(mMeshObject->mFaces[i].mIndices[0]);
		tIndexList.push_back(mMeshObject->mFaces[i].mIndices[1]);
		tIndexList.push_back(mMeshObject->mFaces[i].mIndices[2]);
	}

	return tIndexList;
}
