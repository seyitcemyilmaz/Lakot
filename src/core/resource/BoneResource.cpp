#include "BoneResource.h"

BoneResource::BoneResource(const std::string& pName, const glm::mat4& pTransformation) :
	mName(pName), mTransformation(pTransformation) { }

const std::string& BoneResource::getName() {
	return mName;
}

const glm::mat4& BoneResource::getTransformationMatrix() {
	return mTransformation;
}
