#ifndef LAKOT_ASSIMPTOGLMHELPER_H
#define LAKOT_ASSIMPTOGLMHELPER_H

#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class AssimpToGLMHelper
{
private:
    AssimpToGLMHelper();

public:
    static glm::mat4 toMat4(const aiMatrix4x4& pMatrix);
    static glm::vec3 toVec3(const aiVector3D& pVector);
    static glm::quat toQuat(const aiQuaternion& pAiQuaternion);
};

#endif // ASSIMPTOGLMHELPER_H
