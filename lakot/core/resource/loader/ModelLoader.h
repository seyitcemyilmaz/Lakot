#ifndef LAKOT_MODELLOADER_H
#define LAKOT_MODELLOADER_H

#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include <lakot/utilities/Type.h>
#include <lakot/utilities/DataContainer.hpp>

namespace lakot {

class NodeAsset;
class MeshAsset;
class ModelAsset;
class MaterialAsset;

class ModelLoader
{
public:
    virtual ~ModelLoader();
    ModelLoader();

    ModelAsset* load();

    void setPath(const std::string& pPath);

private:
    std::string mPath;

    ModelAsset* mModelAsset;

    Assimp::Importer* mImporter;
    const aiScene* mSceneObject;

    std::vector<std::string> mPossibleTextureLocations;

    void loadNodeStructure();
    void loadMaterials();

    void loadNode(aiNode* pNode, NodeAsset* pParentNodeAsset);
    void loadMesh(aiMesh* pMesh, NodeAsset* pParentNodeAsset);
    void loadMaterial(aiMaterial* pMaterial);
    void loadTexture(aiTextureType pTextureType, aiMaterial* pMaterial, MaterialAsset* pMaterialAsset);

    Image* createImageFromFile(const std::string& pPath);
    Image* createImageFromEmbedded(const aiTexture* pTexture);
    std::string findImagePath(const std::string& pPath);

    void createPossibleTextureLocations();

    DataContainer* createVertexInformation(aiMesh* pMesh, MeshAsset* pMeshAsset);
};

}

#endif
