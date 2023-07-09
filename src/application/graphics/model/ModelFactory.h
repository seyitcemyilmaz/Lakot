#ifndef MODELFACTORY_H
#define MODELFACTORY_H

#include <string>

#include "Model.h"

class ModelFactory {
private:
	ModelFactory();

	static void processNode(Model* pModel, NodeResource* pNodeResource, Node* pNode);
	static void processMesh(Model* pModel, Node* pConnectedNode, MeshResource* pMeshResource);

public:
	static Model* createModel(std::string pModelFilePath);
};

#endif // MODELFACTORY_H
