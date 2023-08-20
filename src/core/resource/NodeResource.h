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
	void addChildMesh(MeshResource* pChildMesh);
	void addChildNode(NodeResource* pChildNode);
	friend class ModelLoader;

public:
	NodeResource(const std::string& pName, NodeResource* pParentNode, glm::mat4& pTransformationMatrix);

	const std::string& getName() const;

	const std::vector<NodeResource*>& getChildNodes() const;
	const std::vector<MeshResource*>& getChildMeshes() const;

	bool hasChildMesh() const;
};

#endif
