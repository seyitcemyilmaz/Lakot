#include "AnimationLoader.h"

#include "helper/AssimpToGLMHelper.h"

AnimationLoader::AnimationLoader(ModelResource* pModelResource, const aiAnimation* pAnimation)
	: mModelResource(pModelResource)
	, mAnimation(pAnimation) { }

AnimationResource* AnimationLoader::load() {
	std::string tAnimationName = mAnimation->mName.C_Str();

	double tAnimationDuration = mAnimation->mDuration;
	double tAnimationTicksPerSecond = mAnimation->mTicksPerSecond;

	AnimationResource* tAnimationResource = new AnimationResource(tAnimationName, tAnimationDuration, tAnimationTicksPerSecond);

	unsigned int tChannelCount = mAnimation->mNumChannels;

	for (unsigned int j = 0; j < tChannelCount; j++) {
		aiNodeAnim* tChannel = mAnimation->mChannels[j];
		std::string tNodeName = tChannel->mNodeName.C_Str();

		KeyFrameChannel* tKeyFrameChannel = new KeyFrameChannel(tNodeName);
		processKeyFrameChannel(tChannel, tKeyFrameChannel);
		tAnimationResource->addKeyFrameChannel(tKeyFrameChannel);
	}

	return tAnimationResource;
}

void AnimationLoader::processKeyFrameChannel(aiNodeAnim* pAiKeyFrameChannel, KeyFrameChannel* pKeyFrameChannel) {
	unsigned int tKeyFramePositionCount = pAiKeyFrameChannel->mNumPositionKeys;

	for (unsigned int tKeyFramePositionIndex = 0; tKeyFramePositionIndex < tKeyFramePositionCount; tKeyFramePositionIndex++) {
		aiVector3D tPosition = pAiKeyFrameChannel->mPositionKeys[tKeyFramePositionIndex].mValue;
		double tTimeStamp = pAiKeyFrameChannel->mPositionKeys[tKeyFramePositionIndex].mTime;

		KeyFramePosition* tKeyFramePosition = new KeyFramePosition();
		tKeyFramePosition->position = AssimpToGLMHelper::toVec3(tPosition);
		tKeyFramePosition->timeStamp = tTimeStamp;

		pKeyFrameChannel->addToKeyFramePositions(tKeyFramePosition);
	}

	unsigned int tKeyFrameRotationCount = pAiKeyFrameChannel->mNumRotationKeys;

	for (unsigned int tKeyFrameRotationIndex = 0; tKeyFrameRotationIndex < tKeyFrameRotationCount; tKeyFrameRotationIndex++) {
		aiQuaternion tOrientation = pAiKeyFrameChannel->mRotationKeys[tKeyFrameRotationIndex].mValue;
		double tTimeStamp = pAiKeyFrameChannel->mRotationKeys[tKeyFrameRotationIndex].mTime;

		KeyFrameRotation* tKeyFrameRotation = new KeyFrameRotation();
		tKeyFrameRotation->orientation = AssimpToGLMHelper::toQuat(tOrientation);
		tKeyFrameRotation->timeStamp = tTimeStamp;

		pKeyFrameChannel->addToKeyFrameRotations(tKeyFrameRotation);
	}

	unsigned int tKeyFrameScaleCount = pAiKeyFrameChannel->mNumScalingKeys;

	for (unsigned int tKeyFrameScaleIndex = 0; tKeyFrameScaleIndex < tKeyFrameScaleCount; tKeyFrameScaleIndex++) {
		aiVector3D tScale = pAiKeyFrameChannel->mScalingKeys[tKeyFrameScaleIndex].mValue;
		double tTimeStamp = pAiKeyFrameChannel->mScalingKeys[tKeyFrameScaleIndex].mTime;

		KeyFrameScale* tKeyFrameScale = new KeyFrameScale();
		tKeyFrameScale->scale = AssimpToGLMHelper::toVec3(tScale);
		tKeyFrameScale->timeStamp = tTimeStamp;

		pKeyFrameChannel->addToKeyFrameScales(tKeyFrameScale);
	}
}