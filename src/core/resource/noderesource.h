#ifndef LAKOT_NODERESOURCE_H
#define LAKOT_NODERESOURCE_H

#include <iostream>
#include <vector>

#include <glm/glm.hpp>

class NodeResource {
private:
	std::string mName;

	NodeResource* mParentNode;

	std::vector<NodeResource*> mChildNodes;

	glm::mat4 mTransformationMatrix;

protected:
	void addChildNode(NodeResource* pChildNode);
	friend class ModelLoader;

public:
	NodeResource(std::string pName, NodeResource* pParentNode, glm::mat4& pTransformationMatrix);

	std::string getName();
};

#endif
