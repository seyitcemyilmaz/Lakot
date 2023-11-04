#ifndef LAKOT_BONERESOURCE_H
#define LAKOT_BONERESOURCE_H

#include <string>

#include <glm/glm.hpp>

class BoneResource {
private:
	std::string mName;

	glm::mat4 mTransformation;

public:
	BoneResource(const std::string& pName, const glm::mat4& pTransformation);

	const std::string& getName();

	const glm::mat4& getTransformationMatrix();
};

#endif
