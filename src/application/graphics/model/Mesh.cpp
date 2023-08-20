#include "Mesh.h"

Mesh::Mesh(MeshResource* pMeshResource) {
	mMeshResource = pMeshResource;
}

const std::string& Mesh::getName() {
	return mMeshResource->getName();
}

void Mesh::draw(IShader* pShader) {
	mMeshResource->draw(pShader);
}
