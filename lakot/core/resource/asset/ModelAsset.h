#ifndef LAKOT_MODELASSET_H
#define LAKOT_MODELASSET_H

#include <lakot/abstract/resource/asset/AAsset.h>
#include <lakot/abstract/graphics/model/AModel.h>

namespace lakot {

class BoneAsset;
class NodeAsset;
class MeshAsset;
class MaterialAsset;
class TextureAsset;

class ModelAsset
    : public AAsset
    , public AModel
{
public:
    virtual ~ModelAsset() override;
    ModelAsset(const std::string& pPath);

    void initialize() override;
    void deinitialize() override;

    const std::string& getPath() const override;

    const std::vector<AMesh*> getMeshes() const override;
    const std::vector<ANode*> getNodes() const override;
    const std::vector<ABone*> getBones() const override;

    const std::vector<ATexture*> getTextures() const override;
    const std::vector<AMaterial*> getMaterials() const override;

    ANode* getRootNode() const override;

protected:
    friend class ModelLoader;
    void addMesh(MeshAsset* pMesh);
    void addTexture(TextureAsset* pTexture);

private:
    std::string mPath;

    NodeAsset* mRootNode;

    std::vector<AMesh*> mMeshes;
    std::vector<ANode*> mNodes;
    std::vector<AMaterial*> mMaterials;
    std::vector<ATexture*> mTextures;
    std::vector<ABone*> mBones;
};

}

#endif
