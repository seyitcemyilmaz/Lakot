#ifndef LAKOT_MODELLOADER_H
#define LAKOT_MODELLOADER_H

#include <assimp/scene.h>

#include "resource/ModelResource.h"

class ModelLoader {
private:
    ModelResource* mModelResource;

    std::string mModelPath;
    unsigned int mModelImportFlags;

    void extractBones(aiMesh* pMesh, MeshResource* pMeshResource);
    void extractMaterials(const aiScene* pScene);

    void normalizeBoneWeights(std::vector<Vertex>& pVertexList);

    void processMesh(aiMesh* pMesh, NodeResource* pConnectedNodeResource);
    void processNode(aiNode* pNode, const aiScene* pScene, NodeResource* pParentNodeResource);
    void processAnimations(const aiScene* pScene);

    void createMeshBuffers(MeshResource* pMeshResource);

    void applyAxisCorrection(const aiScene* pScene);

public:
    virtual ~ModelLoader();
    ModelLoader(const std::string& pModelPath);

    ModelResource* loadModel();
};

#endif
