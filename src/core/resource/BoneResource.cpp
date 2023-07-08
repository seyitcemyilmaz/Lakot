#include "BoneResource.h"

BoneResource::BoneResource(std::string pName, glm::mat4 pOffsetMatrix) {
	mName = pName;
	mOffsetMatrix = pOffsetMatrix;
}
