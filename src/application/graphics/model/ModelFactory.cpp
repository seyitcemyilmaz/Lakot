#include "ModelFactory.h"

#include "core/resource/ResourceManager.h"

ModelFactory::ModelFactory() = default;

Model* ModelFactory::createModel(std::string pModelFilePath) {
	ModelResource* tModelResource = ResourceManager::getInstance()->getModelResource(pModelFilePath);

	if (!tModelResource) {
		return nullptr;
	}

	NodeResource* tRootNodeResource = tModelResource->getRootNodeResource();
	Node* tRootNode = new Node(tRootNodeResource, nullptr);

	Model* tModel = new Model(tModelResource, tRootNode);
	processNode(tModel, tRootNodeResource, tRootNode);

	return tModel;
}

void ModelFactory::processNode(Model* pModel, NodeResource* pNodeResource, Node* pNode) {
	if (pNodeResource->hasChildMesh()) {
		std::vector<MeshResource*> tChildMeshes = pNodeResource->getChildMeshes();

		for (int i = 0; i < tChildMeshes.size(); i++) {
			processMesh(pModel, pNode, tChildMeshes[i]);
		}
	}

	std::vector<NodeResource*> tChildNodeResources = pNodeResource->getChildNodes();

	for (int i = 0; i < tChildNodeResources.size(); i++) {
		NodeResource* tChildNodeResource = tChildNodeResources[i];
		Node* tChildNode = new Node(tChildNodeResource, pNode);

		pNode->addChildNode(tChildNode);

		processNode(pModel, tChildNodeResource, tChildNode);
	}
}

void ModelFactory::processMesh(Model* pModel, Node* pConnectedNode, MeshResource* pMeshResource) {
	Mesh* tMesh = new Mesh(pMeshResource, pMeshResource->getMaterialIndex());

	pConnectedNode->addChildMesh(tMesh);

	pModel->addMesh(tMesh);
}
