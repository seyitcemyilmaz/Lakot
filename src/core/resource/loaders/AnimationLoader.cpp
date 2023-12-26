#include "AnimationLoader.h"

#include "helper/AssimpToGLMHelper.h"

AnimationLoader::~AnimationLoader()
{

}

AnimationLoader::AnimationLoader(ModelResource* pModelResource, const aiAnimation* pAnimation)
    : mModelResource(pModelResource)
    , mAnimation(pAnimation)
{

}

AnimationResource* AnimationLoader::load()
{
    std::string tAnimationName = mAnimation->mName.C_Str();

    double tAnimationDuration = mAnimation->mDuration;
    double tAnimationTicksPerSecond = mAnimation->mTicksPerSecond;

    AnimationResource* tAnimationResource = new AnimationResource(tAnimationName, tAnimationDuration, tAnimationTicksPerSecond);

    unsigned int tChannelCount = mAnimation->mNumChannels;

    for (unsigned int j = 0; j < tChannelCount; j++)
    {
        aiNodeAnim* tChannel = mAnimation->mChannels[j];

        if (tChannel->mNumPositionKeys == 1 && tChannel->mNumRotationKeys == 1 && tChannel->mNumScalingKeys == 1)
        {
            continue;
        }

        std::string tNodeName = tChannel->mNodeName.C_Str();

        KeyFrameChannelResource* tKeyFrameChannel = new KeyFrameChannelResource(tNodeName);
        processKeyFrameChannel(tChannel, tKeyFrameChannel);
        tAnimationResource->addKeyFrameChannelResource(tKeyFrameChannel);
    }

    return tAnimationResource;
}

void AnimationLoader::processKeyFrameChannel(aiNodeAnim* pAiKeyFrameChannel, KeyFrameChannelResource* pKeyFrameChannel)
{
    unsigned int tKeyFramePositionCount = pAiKeyFrameChannel->mNumPositionKeys;

    for (unsigned int tKeyFramePositionIndex = 0; tKeyFramePositionIndex < tKeyFramePositionCount; tKeyFramePositionIndex++)
    {
        aiVector3D tPosition = pAiKeyFrameChannel->mPositionKeys[tKeyFramePositionIndex].mValue;
        double tTimeStamp = pAiKeyFrameChannel->mPositionKeys[tKeyFramePositionIndex].mTime;

        KeyFramePosition* tKeyFramePosition = new KeyFramePosition();
        tKeyFramePosition->position = AssimpToGLMHelper::toVec3(tPosition);
        tKeyFramePosition->timeStamp = tTimeStamp;

        pKeyFrameChannel->addToKeyFramePositions(tKeyFramePosition);
    }

    unsigned int tKeyFrameRotationCount = pAiKeyFrameChannel->mNumRotationKeys;

    for (unsigned int tKeyFrameRotationIndex = 0; tKeyFrameRotationIndex < tKeyFrameRotationCount; tKeyFrameRotationIndex++)
    {
        aiQuaternion tRotation = pAiKeyFrameChannel->mRotationKeys[tKeyFrameRotationIndex].mValue;
        double tTimeStamp = pAiKeyFrameChannel->mRotationKeys[tKeyFrameRotationIndex].mTime;

        KeyFrameRotation* tKeyFrameRotation = new KeyFrameRotation();
        tKeyFrameRotation->rotation = AssimpToGLMHelper::toQuat(tRotation);
        tKeyFrameRotation->timeStamp = tTimeStamp;

        pKeyFrameChannel->addToKeyFrameRotations(tKeyFrameRotation);
    }

    unsigned int tKeyFrameScaleCount = pAiKeyFrameChannel->mNumScalingKeys;

    for (unsigned int tKeyFrameScaleIndex = 0; tKeyFrameScaleIndex < tKeyFrameScaleCount; tKeyFrameScaleIndex++)
    {
        aiVector3D tScale = pAiKeyFrameChannel->mScalingKeys[tKeyFrameScaleIndex].mValue;
        double tTimeStamp = pAiKeyFrameChannel->mScalingKeys[tKeyFrameScaleIndex].mTime;

        KeyFrameScale* tKeyFrameScale = new KeyFrameScale();
        tKeyFrameScale->scale = AssimpToGLMHelper::toVec3(tScale);
        tKeyFrameScale->timeStamp = tTimeStamp;

        pKeyFrameChannel->addToKeyFrameScales(tKeyFrameScale);
    }
}
