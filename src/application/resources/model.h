#ifndef LAKOT_MODEL_H
#define LAKOT_MODEL_H

#include "mesh.h"

class Model : public IDrawable {
private:
	std::vector<Mesh*> mMeshes;
public:
	Model();

	void addMesh(Mesh* pMesh);

	void draw();
};

#endif
