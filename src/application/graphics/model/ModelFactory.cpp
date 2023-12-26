#include "ModelFactory.h"

#include "core/resource/ResourceManager.h"

ModelFactory::ModelFactory()
{

}

Model* ModelFactory::createModel(const std::string& pModelFilePath)
{
    ModelResource* tModelResource = ResourceManager::getInstance()->getModelResource(pModelFilePath);

    if (!tModelResource)
    {
        return nullptr;
    }

    NodeResource* tRootNodeResource = tModelResource->getRootNodeResource();
    Node* tRootNode = new Node(tRootNodeResource, nullptr, tRootNodeResource->getTransformationMatrix());

    Model* tModel = new Model(tModelResource, tRootNode);

    tModel->mNodes.push_back(tRootNode);

    processNode(tModel, tRootNodeResource, tRootNode);
    processBones(tModel);
    processNodesWithBone(tModel);

    processAnimations(tModel);

    return tModel;
}

void ModelFactory::processNode(Model* pModel, NodeResource* pNodeResource, Node* pNode)
{
    if (pNodeResource->hasChildMesh())
    {
        std::vector<MeshResource*> tChildMeshes = pNodeResource->getChildMeshes();

        for (int i = 0; i < tChildMeshes.size(); i++)
        {
            processMesh(pModel, pNode, tChildMeshes[i]);
        }
    }

    std::vector<NodeResource*> tChildNodeResources = pNodeResource->getChildNodes();

    for (int i = 0; i < tChildNodeResources.size(); i++)
    {
        NodeResource* tChildNodeResource = tChildNodeResources[i];
        Node* tChildNode = new Node(tChildNodeResource, pNode, tChildNodeResource->getTransformationMatrix());

        pModel->mNodes.push_back(tChildNode);

        pNode->addChildNode(tChildNode);

        processNode(pModel, tChildNodeResource, tChildNode);
    }
}

void ModelFactory::processMesh(Model* pModel, Node* pConnectedNode, MeshResource* pMeshResource)
{
    Mesh* tMesh = new Mesh(pMeshResource, pConnectedNode, pMeshResource->getMaterialIndex());

    pConnectedNode->addChildMesh(tMesh);

    pModel->mMeshes.push_back(tMesh);
}

void ModelFactory::processAnimations(Model* pModel)
{
    ModelResource* tModelResource = pModel->getModelResource();

    const std::vector<AnimationResource*>& tAnimationResources = tModelResource->getAnimationResources();
    unsigned int tAnimationCount = tModelResource->getAnimationCount();

    for (unsigned int i = 0; i < tAnimationCount; i++)
    {
        Animation* tAnimation = new Animation(tAnimationResources[i]);

        processKeyFrameChannels(pModel, tAnimation);

        pModel->mAnimations.push_back(tAnimation);
    }
}

void ModelFactory::processKeyFrameChannels(Model* pModel, Animation* pAnimation)
{
    AnimationResource* tAnimationResource = pAnimation->getAnimationResource();

    const std::vector<KeyFrameChannelResource*>& tKeyFrameChannelResources = tAnimationResource->getKeyFrameChannelResources();
    unsigned int tKeyFrameChannelResourceCount = tAnimationResource->getKeyFrameChannelCount();

    for (unsigned int i = 0; i < tKeyFrameChannelResourceCount; i++)
    {
        KeyFrameChannelResource* tKeyFrameChannelResource = tKeyFrameChannelResources[i];

        const std::string& tNodeName = tKeyFrameChannelResource->getNodeName();
        Node* tNode = pModel->getNode(tNodeName);

        KeyFrameChannel* tKeyFrameChannel = new KeyFrameChannel(tKeyFrameChannelResource, tNode);
        pAnimation->addKeyFrameChannel(tKeyFrameChannel);
    }
}

void ModelFactory::processBones(Model* pModel)
{
    ModelResource* tModelResource = pModel->getModelResource();

    const std::vector<BoneResource*>& tBoneResources = tModelResource->getBoneResources();
    unsigned int tBoneCount = tModelResource->getBoneCount();

    for (unsigned int i = 0; i < tBoneCount; i++)
    {
        BoneResource* tBoneResource = tBoneResources[i];

        Bone* tBone = new Bone(tBoneResource, tBoneResource->getTransformationMatrix());

        pModel->mBones.push_back(tBone);
    }
}

void ModelFactory::processMaterials(Model* pModel)
{
    ModelResource* tModelResource = pModel->getModelResource();

    const std::vector<MaterialResource*> tMaterialResources = tModelResource->getMaterialResources();
    unsigned int tMaterialCount = tModelResource->getMaterialCount();

    for (unsigned int i = 0; i < tMaterialCount; i++)
    {
        MaterialResource* tMaterialResource = tMaterialResources[i];

        Material* tMaterial = new Material(tMaterialResource);

        processMaterial(pModel, tMaterial, tMaterialResource);

        pModel->mMaterials.push_back(tMaterial);
    }
}

void ModelFactory::processMaterial(Model* pModel, Material* pMaterial, MaterialResource* pMaterialResource)
{
    // TODO: add necessary functions
}

void ModelFactory::processNodesWithBone(Model* pModel)
{
    std::vector<Node*>& tNodes = pModel->mNodes;
    unsigned int tNodeCount = pModel->getNodeCount();

    for (unsigned int i = 0; i < tNodeCount; i++)
    {
        NodeResource* tNodeResource = tNodes[i]->getNodeResource();
        BoneResource* tBoneResource = tNodeResource->getBone();

        if (tBoneResource)
        {
            unsigned int tIndex = pModel->getModelResource()->getBoneId(tBoneResource);

            Bone* tBone = pModel->mBones[tIndex];

            tNodes[i]->setBone(tBone);
        }
    }
}
