#ifndef LAKOT_MODELLOADER_H
#define LAKOT_MODELLOADER_H

#include <iostream>

#include <assimp/scene.h>

#include "resource/modelresource.h"

class ModelLoader {
private:
	ModelResource* mModelResource;

	std::string mModelPath;
	unsigned int mFlags;

	glm::mat4 assimpMatrixToGlmMatrix(aiMatrix4x4& pMatrix);
	glm::vec3 assimp3DVectorToGlmVector(aiVector3D& pVector);

	std::vector<Vertex> createVertexList(aiMesh* pMesh);
	std::vector<unsigned int> createIndexList(aiMesh* pMesh);

	void extractBones(aiMesh* pMesh, std::vector<Vertex>& pVertexList);
	void extractMaterials(const aiScene* pScene);

	void extractVertexBoneWeights(aiBone* pBone, std::vector<Vertex>& pVertexList);
	void setVertexBoneWeight(Vertex* pVertex, int pBoneId, float pWeight);
	void normalizeVertexBoneWeights(std::vector<Vertex>& pVertexList);

	void processNode(aiNode* pNode, const aiScene* pScene, NodeResource* pParentNodeResource);
	void processMesh(aiMesh* pMesh, const aiScene* pScene, NodeResource* pConnectedNodeResource);

public:
	ModelLoader(std::string pModelPath);

	ModelResource* loadModel();
};

#endif
