#include "Bone.h"

Bone::Bone(BoneResource* pBoneResource, const glm::mat4& pTransformationMatrix)
    : mBoneResource(pBoneResource)
    , mTransformationMatrix(pTransformationMatrix)
{

}

const std::string& Bone::getName() const
{
    return mBoneResource->getName();
}

const glm::mat4& Bone::getTransformationMatrix() const
{
    return mTransformationMatrix;
}

void Bone::setTransformationMatrix(const glm::mat4& pTransformationMatrix)
{
    mTransformationMatrix = pTransformationMatrix;
}

BoneResource* Bone::getBoneResource()
{
    return mBoneResource;
}
