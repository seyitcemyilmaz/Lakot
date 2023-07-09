#ifndef LAKOT_MODEL_H
#define LAKOT_MODEL_H

#include "core/resource/ModelResource.h"

#include "Mesh.h"
#include "Node.h"

class Model {
private:
	ModelResource* mModelResource;

	Node* mRootNode;

	std::vector<Mesh*> mMeshes;

protected:
	void addMesh(Mesh* pMesh);
	friend class ModelFactory;

public:
	Model(ModelResource* pModelResource, Node* pRootNode);

	void draw();
};

#endif
