#include "boneresource.h"

BoneResource::BoneResource(std::string pName, glm::mat4 pOffsetMatrix) {
    mName = pName;
    mOffsetMatrix = pOffsetMatrix;
}
