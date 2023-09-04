#ifndef LAKOT_NODE_H
#define LAKOT_NODE_H

#include "core/resource/NodeResource.h"

#include "Mesh.h"

class Node {
private:
	NodeResource* mNodeResource;

	Node* mParentNode;

	std::vector<Node*> mChildNodes;
	std::vector<Mesh*> mChildMeshes;

protected:
	void addChildNode(Node* pNode);
	void addChildMesh(Mesh* pMesh);
	friend class ModelFactory;

public:
	Node(NodeResource* pNodeResource, Node* pParentNode);

	const std::string& getName();
};

#endif // NODE_H
