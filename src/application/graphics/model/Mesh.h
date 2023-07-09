#ifndef LAKOT_MESH_H
#define LAKOT_MESH_H

#include <vector>

#include "core/resource/MeshResource.h"

#include "IRenderable.h"

class Mesh : public IRenderable {
private:
	MeshResource* mMeshResource;

public:
	Mesh(MeshResource* pMeshResource);

	std::string getName();

	void draw();
};

#endif
