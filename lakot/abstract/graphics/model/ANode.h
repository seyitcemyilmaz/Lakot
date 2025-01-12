#ifndef LAKOT_ANODE_H
#define LAKOT_ANODE_H

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <lakot/utilities/Object.h>

namespace lakot {

class AMesh;

class ANode : public Object
{
public:
    virtual ~ANode() override;
    ANode(ANode* pParentNode, const glm::mat4& pTransformation);

    virtual const std::string& getName() const = 0;

    ANode* getParentNode() const;

    glm::mat4& getTransformation();

    const std::vector<ANode*>& getChildNodes() const;
    const std::vector<AMesh*>& getChildMeshes() const;

protected:
    ANode* mParentNode;

    glm::mat4 mTransformation;

    std::vector<ANode*> mChildNodes;
    std::vector<AMesh*> mChildMeshes;
};

}

#endif
