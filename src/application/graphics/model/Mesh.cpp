#include "Mesh.h"

Mesh::Mesh(MeshResource* pMeshResource, unsigned int pMaterialIndex)
	: mMeshResource(pMeshResource)
	, mMaterialIndex(pMaterialIndex) { }

MeshResource* Mesh::getMeshResource() {
	return mMeshResource;
}

const std::string& Mesh::getName() {
	return mMeshResource->getName();
}

unsigned int Mesh::getMaterialIndex() {
	return mMaterialIndex;
}
