#ifndef LAKOT_NODEASSET_H
#define LAKOT_NODEASSET_H

#include <lakot/abstract/resource/asset/AAsset.h>
#include <lakot/abstract/graphics/model/ANode.h>

namespace lakot {

class MeshAsset;
class AMesh;

class NodeAsset
    : public AAsset
    , public ANode
{
public:
    virtual ~NodeAsset() override;
    NodeAsset(const std::string& pName, NodeAsset* pParentNode, const glm::mat4& pTransformation);

    void initialize() override;
    void deinitialize() override;

    const std::string& getName() const override;

protected:
    friend class ModelLoader;
    void addChildNode(NodeAsset* pNodeAsset);
    void addChildMesh(MeshAsset* pMeshAsset);

private:
    std::string mName;
};

}

#endif
