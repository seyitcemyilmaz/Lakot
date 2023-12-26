#ifndef LAKOT_INODE_H
#define LAKOT_INODE_H

#include <vector>

#include <glm/glm.hpp>

#include "IBone.h"

class INode {
public:
    INode();

    virtual const std::string& getName() const = 0;

    virtual const INode* getParentNode() const = 0;

    virtual const glm::mat4& getTransformationMatrix() const = 0;
    virtual void setTransformationMatrix(const glm::mat4& pTransformationMatrix) = 0;

    virtual const std::vector<INode*>& getChildNodes() const = 0;
    virtual unsigned int getChildNodeCount() const = 0;

    virtual IBone* getBone() const = 0;
    virtual void setBone(IBone* pBone) = 0;
};

#endif
