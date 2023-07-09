#ifndef LAKOT_NODERESOURCE_H
#define LAKOT_NODERESOURCE_H

#include <iostream>
#include <vector>

#include <glm/glm.hpp>

#include "MeshResource.h"

class NodeResource {
private:
	std::string mName;

	NodeResource* mParentNode;

	std::vector<NodeResource*> mChildNodes;
	std::vector<MeshResource*> mChildMeshes;

	glm::mat4 mTransformationMatrix;

protected:
	void addChildNode(NodeResource* pChildNode);
	void addChildMesh(MeshResource* pChildMesh);
	friend class ModelLoader;

public:
	NodeResource(std::string pName, NodeResource* pParentNode, glm::mat4& pTransformationMatrix);

	std::string getName();

	std::vector<NodeResource*> getChildNodes();
	std::vector<MeshResource*> getChildMeshes();

	bool hasChildMesh();
};

#endif
