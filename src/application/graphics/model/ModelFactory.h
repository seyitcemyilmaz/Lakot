#ifndef LAKOT_MODELFACTORY_H
#define LAKOT_MODELFACTORY_H

#include "Model.h"

class ModelFactory {
private:
    ModelFactory();

    static void processNode(Model* pModel, NodeResource* pNodeResource, Node* pNode);
    static void processMesh(Model* pModel, Node* pConnectedNode, MeshResource* pMeshResource);

    static void processAnimations(Model* pModel);
    static void processKeyFrameChannels(Model* pModel, Animation* pAnimation);
    static void processBones(Model* pModel);

    static void processMaterials(Model* pModel);
    static void processMaterial(Model* pModel, Material* pMaterial, MaterialResource* pMaterialResource);
    static void processNodesWithBone(Model* pModel);

public:
    static Model* createModel(const std::string& pModelFilePath);
};

#endif
