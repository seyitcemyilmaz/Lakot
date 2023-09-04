#ifndef LAKOT_MODELFACTORY_H
#define LAKOT_MODELFACTORY_H

#include <string>

#include "Model.h"

class ModelFactory {
private:
	ModelFactory();

	static void processNode(Model* pModel, NodeResource* pNodeResource, Node* pNode);
	static void processMesh(Model* pModel, Node* pConnectedNode, MeshResource* pMeshResource);
	static void processAnimations(Model* pModel);

public:
	static Model* createModel(std::string pModelFilePath);
};

#endif // MODELFACTORY_H
