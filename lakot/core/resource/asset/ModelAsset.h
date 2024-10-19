#ifndef LAKOT_MODELASSET_H
#define LAKOT_MODELASSET_H

#include <lakot/abstract/resource/asset/AAsset.h>

namespace lakot {

class BoneAsset;
class NodeAsset;
class MeshAsset;
class MaterialAsset;
class TextureAsset;

class ModelAsset : public AAsset
{
public:
    virtual ~ModelAsset() override;
    ModelAsset(const std::string& pPath);

    void initialize() override;
    void deinitialize() override;

    NodeAsset* getRootNode() const;

    const std::vector<TextureAsset*>& getTextures() const;

protected:
    friend class ModelLoader;
    void addMesh(MeshAsset* pMesh);
    void addTexture(TextureAsset* pTexture);

private:
    std::string mPath;

    NodeAsset* mRootNode;

    std::vector<MeshAsset*> mMeshes;
    std::vector<MaterialAsset*> mMaterials;
    std::vector<TextureAsset*> mTextures;
    std::vector<BoneAsset*> mBones;
};

}

#endif
