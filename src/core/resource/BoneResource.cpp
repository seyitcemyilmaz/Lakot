#include "BoneResource.h"

BoneResource::BoneResource(const std::string& pName, const glm::mat4& pOffsetMatrix) :
	mName(pName), mOffsetMatrix(pOffsetMatrix) { }
