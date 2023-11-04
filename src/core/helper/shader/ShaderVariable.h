#ifndef LAKOT_SHADERVARIABLE_H
#define LAKOT_SHADERVARIABLE_H

#include <string>

#include <glm/glm.hpp>

enum class ShaderVariableName {
	eView,
	eProjection,
	eModel,

	eDiffuseTexture,
	eNormalsTexture,
	eSpecularTexture,
	eEmissiveTexture,
	eAmbientTexture,
	eMetalnessTexture,

	eHasDiffuseTexture,
	eHasNormalsTexture,
	eHasSpecularTexture,
	eHasEmissiveTexture,
	eHasAmbientTexture,
	eHasMetalnessTexture,

	eDiffuseColor,
	eSpecularColor,
	eEmissiveColor,
	eAmbientColor,

	eAnimationType,
	eBoneTransformations
};

enum class ShaderVariableDataType {
	eSampler2D,

	eBool,
	eInt,
	eMat4,
	eMat4Array,
	eVec3,
};

class ShaderVariable {
private:
	std::string mName;

	ShaderVariableDataType mDataType;

	int mLocation = -1;

public:
	ShaderVariable(const std::string& pName, ShaderVariableDataType pDataType);

	const std::string& getName() const;

	int getLocation() const;
	void setLocation(int pLocation);

	void setBool(bool pValue) const;
	void setInt(int pValue) const;
	void setMat4(const glm::mat4& pValue) const;
	void setMat4Array(const glm::mat4* pArray, unsigned int pCount) const;
	void setVec3(const glm::vec3& pValue) const;

	void setTexture(unsigned int pUnit, int pTextureId) const;
};

#endif // SHADERVARIABLE_H
