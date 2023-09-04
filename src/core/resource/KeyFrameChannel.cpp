#include "KeyFrameChannel.h"

#include "glm/gtx/quaternion.hpp"

KeyFrameChannel::KeyFrameChannel(std::string pNodeName) : mNodeName(pNodeName) { }

const std::string& KeyFrameChannel::getNodeName() const {
	return mNodeName;
}

void KeyFrameChannel::addToKeyFramePositions(KeyFramePosition* pKeyFramePosition) {
	mKeyFramePositions.push_back(pKeyFramePosition);
}

void KeyFrameChannel::addToKeyFrameRotations(KeyFrameRotation* pKeyFrameRotation) {
	mKeyFrameRotations.push_back(pKeyFrameRotation);
}

void KeyFrameChannel::addToKeyFrameScales(KeyFrameScale* pKeyFrameScale) {
	mKeyFrameScales.push_back(pKeyFrameScale);
}

unsigned int KeyFrameChannel::getKeyFramePositionsCount() const {
	return static_cast<unsigned int>(mKeyFramePositions.size());
}

unsigned int KeyFrameChannel::getKeyFrameRotationsCount() const {
	return static_cast<unsigned int>(mKeyFrameRotations.size());
}

unsigned int KeyFrameChannel::getKeyFrameScalesCount() const {
	return static_cast<unsigned int>(mKeyFrameScales.size());
}

const std::vector<KeyFramePosition*>& KeyFrameChannel::getKeyFramePositions() const {
	return mKeyFramePositions;
}

const std::vector<KeyFrameRotation*>& KeyFrameChannel::getKeyFrameRotations() const {
	return mKeyFrameRotations;
}

const std::vector<KeyFrameScale*>& KeyFrameChannel::getKeyFrameScales() const {
	return mKeyFrameScales;
}

int KeyFrameChannel::getPositionIndex(double pAnimationTime) const {
	for (int tIndex = 0; tIndex < mKeyFramePositions.size() - 1; tIndex++) {		///TODO: "- 1" may cause some problems, it should be checked.
		if (pAnimationTime < mKeyFramePositions[tIndex + 1]->timeStamp) {
			return tIndex;
		}
	}
	return -1;
}

int KeyFrameChannel::getRotationIndex(double pAnimationTime) const {
	for (int tIndex = 0; tIndex < mKeyFrameRotations.size() - 1; tIndex++) {		///TODO: "- 1" may cause some problems, it should be checked.
		if (pAnimationTime < mKeyFrameRotations[tIndex + 1]->timeStamp) {
			return tIndex;
		}
	}
	return -1;
}

int KeyFrameChannel::getScaleIndex(double pAnimationTime) const {
	for (int tIndex = 0; tIndex < mKeyFrameScales.size() - 1; tIndex++) {			///TODO: "- 1" may cause some problems, it should be checked.
		if (pAnimationTime < mKeyFrameScales[tIndex + 1]->timeStamp) {
			return tIndex;
		}
	}
	return -1;
}

double KeyFrameChannel::getScaleFactor(double pLastTimeStamp, double pNextTimeStamp, double pAnimationTime) const {
	double tScaleFactor = 0.0;

	double tMidWayLength = std::abs(pAnimationTime - pLastTimeStamp);
	double tFramesDifference = std::abs(pNextTimeStamp - pLastTimeStamp);

	tScaleFactor = tMidWayLength / tFramesDifference;

	return tScaleFactor;
}

glm::mat4 KeyFrameChannel::interpolatePosition(double pAnimationTime) const {
	if (getKeyFramePositionsCount() == 1) {
		return glm::translate(glm::mat4(1.0f), mKeyFramePositions[0]->position);
	}

	int tP0Index = getPositionIndex(pAnimationTime);
	int tP1Index = tP0Index + 1;

	if (tP0Index == -1) {
		return glm::translate(glm::mat4(1.0f), mKeyFramePositions[getKeyFramePositionsCount() - 1]->position);
	}

	double tScaleFactor = getScaleFactor(mKeyFramePositions[tP0Index]->timeStamp, mKeyFramePositions[tP1Index]->timeStamp, pAnimationTime);

	glm::vec3 tFinalPosition = glm::mix(mKeyFramePositions[tP0Index]->position, mKeyFramePositions[tP1Index]->position, tScaleFactor);
	return glm::translate(glm::mat4(1.0f), tFinalPosition);
}

glm::mat4 KeyFrameChannel::interpolateRotation(double pAnimationTime) const {
	if (getKeyFrameRotationsCount() == 1) {
		auto tRotation = glm::normalize(mKeyFrameRotations[0]->orientation);
		return glm::toMat4(tRotation);
	}

	int tP0Index = getRotationIndex(pAnimationTime);
	int tP1Index = tP0Index + 1;

	if (tP0Index == -1) {
		return glm::toMat4(glm::normalize(mKeyFrameRotations[getKeyFrameRotationsCount() - 1]->orientation));
	}

	double tScaleFactor = getScaleFactor(mKeyFrameRotations[tP0Index]->timeStamp, mKeyFrameRotations[tP1Index]->timeStamp, pAnimationTime);

	glm::quat tFinalRotation = glm::slerp(mKeyFrameRotations[tP0Index]->orientation, mKeyFrameRotations[tP1Index]->orientation, static_cast<float>(tScaleFactor));
	tFinalRotation = glm::normalize(tFinalRotation);
	return glm::toMat4(tFinalRotation);
}

glm::mat4 KeyFrameChannel::interpolateScale(double pAnimationTime) const {
	if (getKeyFrameScalesCount() == 1) {
		return glm::scale(glm::mat4(1.0f), mKeyFrameScales[0]->scale);
	}

	int tP0Index = getScaleIndex(pAnimationTime);
	int tP1Index = tP0Index + 1;

	if (tP0Index == -1) {
		return glm::scale(glm::mat4(1.0f), mKeyFrameScales[getKeyFrameScalesCount() - 1]->scale);
	}

	double tScaleFactor = getScaleFactor(mKeyFrameScales[tP0Index]->timeStamp, mKeyFrameScales[tP1Index]->timeStamp, pAnimationTime);

	glm::vec3 tFinalScale = glm::mix(mKeyFrameScales[tP0Index]->scale, mKeyFrameScales[tP1Index]->scale, tScaleFactor);
	return glm::scale(glm::mat4(1.0f), tFinalScale);
}
