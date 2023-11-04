#include "ITransformable.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

ITransformable::ITransformable(const glm::vec3& pPosition, const glm::vec3& pScale, const glm::vec3& pRotation)
	: mPosition(pPosition)
	, mScale(pScale)
	, mRotation(pRotation) { }

const glm::vec3& ITransformable::getPosition() {
	return mPosition;
}

const glm::vec3& ITransformable::getScale() {
	return mScale;
}

const glm::vec3& ITransformable::getRotation() {
	return mRotation;
}

void ITransformable::setPosition(const glm::vec3& pPosition) {
	mPosition = pPosition;
}

void ITransformable::setScale(const glm::vec3& pScale) {
	mScale = pScale;
}

void ITransformable::setRotation(const glm::vec3& pRotation) {
	mRotation = pRotation;
}

void ITransformable::setPosition(float pX, float pY, float pZ) {
	mPosition.x = pX;
	mPosition.y = pY;
	mPosition.z = pZ;
}

void ITransformable::setScale(float pX, float pY, float pZ) {
	mScale.x = pX;
	mScale.y = pY;
	mScale.z = pZ;
}

void ITransformable::setRotation(float pX, float pY, float pZ) {
	mRotation.x = pX;
	mRotation.y = pY;
	mRotation.z = pZ;
}

void ITransformable::calculateModelMatrix() {
	mModelMatrix = glm::translate(glm::mat4(1.0f), mPosition)
				   * glm::toMat4(glm::quat(glm::vec3(glm::radians(mRotation.x), glm::radians(mRotation.y), glm::radians(mRotation.z))))
				   * glm::scale(glm::mat4(1.0f), mScale);

	mInverseTransposeModelMatrix = glm::inverseTranspose(glm::mat3(mModelMatrix));
}

const glm::mat4& ITransformable::getModelMatrix() const {
	return mModelMatrix;
}

const glm::mat3& ITransformable::getInverseTransposeModelMatrix() const {
	return mInverseTransposeModelMatrix;
}

void ITransformable::changePositionX(float pAmount) {
	mPosition.x += pAmount;
}

void ITransformable::changePositionY(float pAmount) {
	mPosition.y += pAmount;
}

void ITransformable::changePositionZ(float pAmount) {
	mPosition.z += pAmount;
}

void ITransformable::changePosition(const glm::vec3& pAmount) {
	mPosition += pAmount;
}

void ITransformable::changePosition(float pAmountX, float pAmountY, float pAmountZ) {
	mPosition.x += pAmountX;
	mPosition.y += pAmountY;
	mPosition.z += pAmountZ;
}

void ITransformable::changeScaleX(float pAmount) {
	mScale.x += pAmount;
}

void ITransformable::changeScaleY(float pAmount) {
	mScale.y += pAmount;
}

void ITransformable::changeScaleZ(float pAmount) {
	mScale.z += pAmount;
}

void ITransformable::changeScale(const glm::vec3& pAmount) {
	mScale += pAmount;
}

void ITransformable::changeScale(float pAmountX, float pAmountY, float pAmountZ) {
	mScale.x += pAmountX;
	mScale.y += pAmountY;
	mScale.z += pAmountZ;
}

void ITransformable::changeRotationX(float pAmount) {
	mRotation.x += pAmount;
}

void ITransformable::changeRotationY(float pAmount) {
	mRotation.y += pAmount;
}

void ITransformable::changeRotationZ(float pAmount) {
	mRotation.z += pAmount;
}

void ITransformable::changeRotation(const glm::vec3& pAmount) {
	mRotation += pAmount;
}

void ITransformable::changeRotation(float pAmountX, float pAmountY, float pAmountZ) {
	mRotation.x += pAmountX;
	mRotation.y += pAmountY;
	mRotation.z += pAmountZ;
}
