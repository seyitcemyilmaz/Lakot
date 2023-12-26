#ifndef LAKOT_NODE_H
#define LAKOT_NODE_H

#include "interface/INode.h"

#include "core/resource/NodeResource.h"

#include "Mesh.h"

class Node : public INode
{
private:
    NodeResource* mNodeResource;

    Node* mParentNode;

    std::vector<INode*> mChildNodes;
    std::vector<Mesh*> mChildMeshes;

    glm::mat4 mTransformationMatrix;

    IBone* mBone;

protected:
    void addChildNode(Node* pNode);
    void addChildMesh(Mesh* pMesh);

    friend class ModelFactory;

public:
    Node(NodeResource* pNodeResource, Node* pParentNode, const glm::mat4& pTransformationMatrix);

    const std::string& getName() const override;

    const INode* getParentNode() const override;

    const glm::mat4& getTransformationMatrix() const override;
    void setTransformationMatrix(const glm::mat4& pTransformationMatrix) override;

    const std::vector<INode*>& getChildNodes() const override;
    unsigned int getChildNodeCount() const override;

    IBone* getBone() const override;
    void setBone(IBone* pBone) override;

    NodeResource* getNodeResource();
};

#endif
