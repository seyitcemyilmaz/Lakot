#ifndef LAKOT_MODELLOADER_H
#define LAKOT_MODELLOADER_H

#include <iostream>

#include <assimp/scene.h>

#include "resource/ModelResource.h"

class ModelLoader {
private:
	ModelResource* mModelResource;

	std::string mModelPath;
	unsigned int mFlags;

	void extractBones(aiMesh* pMesh, MeshResource* pMeshResource);
	void extractMaterials(const aiScene* pScene);

	void normalizeBoneWeights(std::vector<Vertex>& pVertexList);

	void processMesh(aiMesh* pMesh, NodeResource* pConnectedNodeResource);
	void processNode(aiNode* pNode, const aiScene* pScene, NodeResource* pParentNodeResource);

public:
	ModelLoader(const std::string& pModelPath);

	ModelResource* loadModel();
};

#endif
