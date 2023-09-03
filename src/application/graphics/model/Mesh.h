#ifndef LAKOT_MESH_H
#define LAKOT_MESH_H

#include <vector>

#include "core/resource/MeshResource.h"

class Mesh {
private:
	MeshResource* mMeshResource;

	unsigned int mMaterialIndex = -1;

protected:
	friend class RenderManager;

public:
	Mesh(MeshResource* pMeshResource, unsigned int pMaterialIndex);

	const std::string& getName();

	MeshResource* getMeshResource();

	unsigned int getMaterialIndex();
};

#endif
