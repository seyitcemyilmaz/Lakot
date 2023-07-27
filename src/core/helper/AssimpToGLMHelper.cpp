#include "AssimpToGLMHelper.h"

AssimpToGLMHelper::AssimpToGLMHelper() { }

glm::mat4 AssimpToGLMHelper::toMat4(aiMatrix4x4& pMatrix) {
	return glm::mat4 {
		pMatrix.a1, pMatrix.b1, pMatrix.c1, pMatrix.d1,
		pMatrix.a2, pMatrix.b2, pMatrix.c2, pMatrix.d2,
		pMatrix.a3, pMatrix.b3, pMatrix.c3, pMatrix.d3,
		pMatrix.a4, pMatrix.b4, pMatrix.c4, pMatrix.d4
	};
}

glm::vec3 AssimpToGLMHelper::toVec3(aiVector3D& pVector) {
	return glm::vec3 {
		pVector.x, pVector.y, pVector.z
	};
}
