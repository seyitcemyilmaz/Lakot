#ifndef MESHLOADER_H
#define MESHLOADER_H

#include <assimp/scene.h>

#include "resource/MeshResource.h"

class MeshLoader {
private:
	aiMesh* mMeshObject;

	std::vector<Vertex> createVertexList();
	std::vector<unsigned int> createIndexList();

public:
	MeshLoader(aiMesh* pMeshObject);

	MeshResource* loadMesh();
};

#endif
