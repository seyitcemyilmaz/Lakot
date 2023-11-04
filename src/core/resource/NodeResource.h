#ifndef LAKOT_NODERESOURCE_H
#define LAKOT_NODERESOURCE_H

#include <iostream>
#include <vector>

#include <glm/glm.hpp>

#include "MeshResource.h"
#include "BoneResource.h"

class NodeResource {
private:
	std::string mName;

	NodeResource* mParentNode;

	std::vector<NodeResource*> mChildNodes;
	std::vector<MeshResource*> mChildMeshes;

	glm::mat4 mTransformationMatrix;

	BoneResource* mBoneResource = nullptr;

protected:
	void setBoneResource(BoneResource* pBoneResource);

	void addChildMesh(MeshResource* pChildMesh);
	void addChildNode(NodeResource* pChildNode);
	friend class ModelLoader;

public:
	NodeResource(const std::string& pName, NodeResource* pParentNode, glm::mat4& pTransformationMatrix);

	const std::string& getName() const;

	const std::vector<NodeResource*>& getChildNodes() const;
	const std::vector<MeshResource*>& getChildMeshes() const;

	BoneResource* getBone();

	bool hasChildMesh() const;

	const glm::mat4& getTransformationMatrix();
};

#endif
