#include "Mesh.h"

Mesh::Mesh(MeshResource* pMeshResource) {
	mMeshResource = pMeshResource;
}

std::string Mesh::getName() {
	return mMeshResource->getName();
}

void Mesh::draw() {
	mMeshResource->draw();
}