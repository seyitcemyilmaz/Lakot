#ifndef ASSIMPTOGLMHELPER_H
#define ASSIMPTOGLMHELPER_H

#include <glm/glm.hpp>
#include <assimp/matrix4x4.h>

class AssimpToGLMHelper
{
private:
	AssimpToGLMHelper();
public:
	static glm::mat4 toMat4(aiMatrix4x4& pMatrix);
	static glm::vec3 toVec3(aiVector3D& pVector);
};

#endif // ASSIMPTOGLMHELPER_H