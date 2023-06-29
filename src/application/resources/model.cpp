#include "model.h"

Model::Model() { }

void Model::addMesh(Mesh* pMesh) {
	mMeshes.push_back(pMesh);
}

void Model::draw() {
	for (int i = 0; i < mMeshes.size(); i++) {
		mMeshes[i]->draw();
	}
}
