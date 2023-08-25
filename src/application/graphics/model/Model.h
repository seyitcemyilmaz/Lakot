#ifndef LAKOT_MODEL_H
#define LAKOT_MODEL_H

#include "core/resource/ModelResource.h"

#include "Mesh.h"
#include "Node.h"

#include "interface/ITransformable.h"

class Model : public ITransformable {
private:
	ModelResource* mModelResource;

	Node* mRootNode;

	std::vector<Mesh*> mMeshes;

protected:
	void addMesh(Mesh* pMesh);
	friend class ModelFactory;

public:
	Model(ModelResource* pModelResource, Node* pRootNode);

	void draw(IShader* pShader);



};

#endif
