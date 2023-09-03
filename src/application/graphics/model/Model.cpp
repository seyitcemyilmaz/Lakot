#include "Model.h"

Model::Model(ModelResource* pModelResource, Node* pRootNode)
	: ITransformable(glm::vec3(0.0f), glm::vec3(1.0f), glm::vec3(0.0f))
	, mModelResource(pModelResource)
	, mRootNode(pRootNode) { }

ModelResource* Model::getModelResource() {
	return mModelResource;
}

void Model::addMesh(Mesh* pMesh) {
	mMeshes.push_back(pMesh);
}

const std::vector<Mesh*>& Model::getMeshes() {
	return mMeshes;
}
