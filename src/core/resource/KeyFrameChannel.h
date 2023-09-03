#ifndef LAKOT_KEYFRAMECHANNEL_H
#define LAKOT_KEYFRAMECHANNEL_H

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

struct KeyFramePosition {
	glm::vec3 position;
	double timeStamp;
};

struct KeyFrameRotation {
	glm::quat orientation;
	double timeStamp;
};

struct KeyFrameScale {
	glm::vec3 scale;
	double timeStamp;
};

class KeyFrameChannel {
private:
	std::string mNodeName;

	std::vector<KeyFramePosition*> mKeyFramePositions;
	std::vector<KeyFrameRotation*> mKeyFrameRotations;
	std::vector<KeyFrameScale*> mKeyFrameScales;

	double getScaleFactor(double pLastTimeStamp, double pNextTimeStamp, double pAnimationTime) const;

	int getPositionIndex(double pAnimationTime) const;
	int getRotationIndex(double pAnimationTime) const;
	int getScaleIndex(double pAnimationTime) const;

public:
	KeyFrameChannel(std::string pNodeName);

	const std::string& getNodeName() const;

	int getKeyFramePositionsCount() const;
	int getKeyFrameRotationsCount() const;
	int getKeyFrameScalesCount() const;

	void addToKeyFramePositions(KeyFramePosition* pKeyFramePosition);
	void addToKeyFrameRotations(KeyFrameRotation* pKeyFrameRotation);
	void addToKeyFrameScales(KeyFrameScale* pKeyFrameScale);

	const std::vector<KeyFramePosition*>& getKeyFramePositions() const;
	const std::vector<KeyFrameRotation*>& getKeyFrameRotations() const;
	const std::vector<KeyFrameScale*>& getKeyFrameScales() const;

	glm::mat4 interpolatePosition(double pAnimationTime) const;
	glm::mat4 interpolateRotation(double pAnimationTime) const;
	glm::mat4 interpolateScale(double pAnimationTime) const;
};

#endif // KEYFRAMECHANNEL_H
