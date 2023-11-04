#ifndef LAKOT_ITRANSFORMABLE_H
#define LAKOT_ITRANSFORMABLE_H

#include <glm/glm.hpp>

class ITransformable {
private:
	glm::vec3 mPosition;
	glm::vec3 mScale;
	glm::vec3 mRotation;

	glm::mat4 mModelMatrix;
	glm::mat3 mInverseTransposeModelMatrix;

public:
	ITransformable(const glm::vec3& pPosition, const glm::vec3& pScale, const glm::vec3& pRotation);

	const glm::vec3& getPosition();
	const glm::vec3& getScale();
	const glm::vec3& getRotation();

	void setPosition(const glm::vec3& pPosition);
	void setScale(const glm::vec3& pScale);
	void setRotation(const glm::vec3& pRotation);

	void setPosition(float pX, float pY, float pZ);
	void setScale(float pX, float pY, float pZ);
	void setRotation(float pX, float pY, float pZ);

	void changePositionX(float pAmount);
	void changePositionY(float pAmount);
	void changePositionZ(float pAmount);
	void changePosition(const glm::vec3& pAmount);
	void changePosition(float pAmountX, float pAmountY, float pAmountZ);

	void changeScaleX(float pAmount);
	void changeScaleY(float pAmount);
	void changeScaleZ(float pAmount);
	void changeScale(const glm::vec3& pAmount);
	void changeScale(float pAmountX, float pAmountY, float pAmountZ);

	void changeRotationX(float pAmount);
	void changeRotationY(float pAmount);
	void changeRotationZ(float pAmount);
	void changeRotation(const glm::vec3& pAmount);
	void changeRotation(float pAmountX, float pAmountY, float pAmountZ);

	void calculateModelMatrix();
	const glm::mat4& getModelMatrix() const;
	const glm::mat3& getInverseTransposeModelMatrix() const;
};

#endif
