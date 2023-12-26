#include "BoneResource.h"

BoneResource::BoneResource(const std::string& pName, const glm::mat4& pTransformation)
    : mName(pName)
    , mTransformation(pTransformation)
{

}

const std::string& BoneResource::getName() const
{
    return mName;
}

const glm::mat4& BoneResource::getTransformationMatrix() const
{
    return mTransformation;
}
