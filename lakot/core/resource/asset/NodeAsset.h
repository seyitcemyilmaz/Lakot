#ifndef LAKOT_NODEASSET_H
#define LAKOT_NODEASSET_H

#include <vector>

#include <glm/glm.hpp>

#include <lakot/abstract/resource/asset/AAsset.h>

namespace lakot {

class MeshAsset;

class NodeAsset : public AAsset
{
public:
    virtual ~NodeAsset() override;
    NodeAsset(const std::string& pName, const glm::mat4& pTransform, NodeAsset* pParentNode);

    void initialize() override;
    void deinitialize() override;

    const std::string& getName() const;

    const glm::mat4& getTransform() const;

    NodeAsset* getParentNode() const;

    const std::vector<NodeAsset*>& getChildNodes() const;
    const std::vector<MeshAsset*>& getChildMeshes() const;

protected:
    friend class ModelLoader;
    void addChildNode(NodeAsset* pNodeAsset);
    void addChildMesh(MeshAsset* pMeshAsset);

private:
    std::string mName;

    glm::mat4 mTransform;

    NodeAsset* mParentNode;

    std::vector<NodeAsset*> mChildNodes;
    std::vector<MeshAsset*> mChildMeshes;
};

}

#endif
