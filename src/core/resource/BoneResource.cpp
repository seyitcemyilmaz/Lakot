#include "BoneResource.h"

BoneResource::BoneResource(const std::string& pName, const glm::mat4& pOffsetMatrix) :
	mName(pName), mOffsetMatrix(pOffsetMatrix) { }

const std::string& BoneResource::getName() {
	return mName;
}

const glm::mat4& BoneResource::getOffsetMatrix() {
	return mOffsetMatrix;
}
