#include "Model.h"

#include <spdlog/spdlog.h>

Model::Model(ModelResource* pModelResource, Node* pRootNode)
    : ITransformable(glm::vec3(0.0f), glm::vec3(1.0f), glm::vec3(0.0f))
    , mModelResource(pModelResource)
    , mRootNode(pRootNode)
{

}

void Model::calculateBoneTransformations(INode* pNode, const glm::mat4& tParentTransform)
{
    const glm::mat4& tNodeTransformationMatrix = pNode->getTransformationMatrix();

    glm::mat4 tGlobalTransform = tParentTransform * tNodeTransformationMatrix;

    Bone* tBone = static_cast<Bone*>(pNode->getBone());

    if (tBone)
    {
        const glm::mat4& tBoneResourceTransformationMatrix = tBone->getBoneResource()->getTransformationMatrix();

        glm::mat4 tBoneTransformationMatrix = mModelResource->getGlobalInverseTransform() *
                                              tGlobalTransform *
                                              tBoneResourceTransformationMatrix;

        tBone->setTransformationMatrix(tBoneTransformationMatrix);
    }

    const std::vector<INode*>& tChildNodes = pNode->getChildNodes();
    unsigned int tChildNodeCount = pNode->getChildNodeCount();

    for (unsigned int i = 0; i < tChildNodeCount; i++)
    {
        calculateBoneTransformations(tChildNodes[i], tGlobalTransform);
    }
}

void Model::updateAnimations(double pTimeDifference)
{
    unsigned int tActiveAnimationCount = getActiveAnimationCount();

    for (unsigned int i = 0; i < tActiveAnimationCount; i++)
    {
        Animation* tAnimation = mActiveAnimations[i];

        if (tAnimation->getAnimationStatus() == AnimationStatus::ePlay)
        {
            tAnimation->update(pTimeDifference);

            if (tAnimation->getAnimationStatus() == AnimationStatus::eStop)
            {
                mActiveAnimations.erase(mActiveAnimations.begin() + i);
                tActiveAnimationCount--;
                i--;
            }
        }
    }
}

void Model::playAnimationContinuously(unsigned int pIndex)
{
    if (pIndex >= getAnimationCount())
    {
        spdlog::error("Invalid animation index.");
        return;
    }

    Animation* tAnimation = mAnimations[pIndex];

    auto tAnimatorIterator = std::find(mActiveAnimations.begin(),
                                       mActiveAnimations.end(),
                                       tAnimation);

    if (tAnimatorIterator != mActiveAnimations.end())
    {
        return;
    }

    tAnimation->playAnimation(AnimationPlayType::eContinuously);
    mActiveAnimations.push_back(tAnimation);
}

void Model::playAnimationOnlyOnce(unsigned int pIndex)
{
    if (pIndex >= getAnimationCount())
    {
        spdlog::warn("Invalid animation index.");
        return;
    }

    Animation* tAnimation = mAnimations[pIndex];

    auto tAnimatorIterator = std::find(mActiveAnimations.begin(),
                                       mActiveAnimations.end(),
                                       tAnimation);

    if (tAnimatorIterator != mActiveAnimations.end())
    {
        return;
    }

    tAnimation->playAnimation(AnimationPlayType::eOnlyOnce);
    mActiveAnimations.push_back(tAnimation);
}

void Model::pauseAnimation(unsigned int pIndex)
{
    if (pIndex >= getAnimationCount())
    {
        spdlog::warn("Invalid animation index.");
        return;
    }

    Animation* tAnimation = mAnimations[pIndex];

    auto tAnimatorIterator = std::find(mActiveAnimations.begin(),
                                       mActiveAnimations.end(),
                                       tAnimation);

    if (tAnimatorIterator == mActiveAnimations.end())
    {
        return;
    }

    tAnimation->pauseAnimation();
}

void Model::stopAnimation(unsigned int pIndex)
{
    if (pIndex >= getAnimationCount())
    {
        spdlog::warn("Invalid animation index.");
        return;
    }

    Animation* tAnimation = mAnimations[pIndex];

    auto tAnimatorIterator = std::find(mActiveAnimations.begin(),
                                       mActiveAnimations.end(),
                                       tAnimation);

    if (tAnimatorIterator == mActiveAnimations.end())
    {
        return;
    }

    tAnimation->stopAnimation();
}

void Model::update(double pTimeDifference)
{
    updateAnimations(pTimeDifference);
    calculateBoneTransformations(mRootNode, glm::mat4(1.0f));

    unsigned int tMeshCount = getMeshCount();

    for (unsigned int i = 0; i < tMeshCount; i++)
    {
        mMeshes[i]->calculateTransformationMatrix();
    }
}

Node* Model::getNode(const std::string& pNodeName)
{
    unsigned int tNodeCount = getNodeCount();

    Node* pNode = nullptr;

    for (unsigned int i = 0; i < tNodeCount; i++)
    {
        if (mNodes[i]->getName() == pNodeName)
        {
            pNode = mNodes[i];
            break;
        }
    }

    return pNode;
}

ModelResource* Model::getModelResource()
{
    return mModelResource;
}

Node* Model::getRootNode()
{
    return mRootNode;
}

const glm::mat4& Model::getGlobalInverseTransform()
{
    return mModelResource->getGlobalInverseTransform();
}

unsigned int Model::getMeshCount() const
{
    return static_cast<unsigned int>(mMeshes.size());
}

unsigned int Model::getNodeCount() const
{
    return static_cast<unsigned int>(mNodes.size());
}

unsigned int Model::getBoneCount() const
{
    return static_cast<unsigned int>(mBones.size());
}

unsigned int Model::getAnimationCount() const
{
    return static_cast<unsigned int>(mAnimations.size());
}

unsigned int Model::getActiveAnimationCount() const
{
    return static_cast<unsigned int>(mActiveAnimations.size());
}

bool Model::getHasActiveAnimation() const
{
    return !mActiveAnimations.empty();
}

std::vector<glm::mat4> Model::getBoneMatrices() const
{
    unsigned int tBoneCount = getBoneCount();

    std::vector<glm::mat4> tBoneMatrices;

    for (unsigned int i = 0; i < tBoneCount; i++)
    {
        tBoneMatrices.push_back(mBones[i]->getTransformationMatrix());
    }

    return tBoneMatrices;
}

const std::vector<Mesh*>& Model::getMeshes()
{
    return mMeshes;
}

const std::vector<Bone*>& Model::getBones()
{
    return mBones;
}

const std::vector<Node*>& Model::getNodes()
{
    return mNodes;
}

const std::vector<Material*>& Model::getMaterials()
{
    return mMaterials;
}
