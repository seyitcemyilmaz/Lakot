#include "AnimationRetargeter.h"

#include <algorithm>
#include <cmath>

using namespace lakot;

namespace
{
    const char* const kRootJoint = "Hips";

    std::vector<glm::quat> toGlobalRotations(const Skeleton& pSkeleton, const std::vector<JointPose>& pPoses)
    {
        const auto& tJoints = pSkeleton.getJoints();
        std::vector<glm::quat> tGlobal(tJoints.size());

        for (size_t tIndex = 0; tIndex < tJoints.size(); ++tIndex)
        {
            int tParent = tJoints[tIndex].parent;
            glm::quat tLocal = glm::normalize(pPoses[tIndex].rotation);
            tGlobal[tIndex] = tParent >= 0 ? tGlobal[static_cast<size_t>(tParent)] * tLocal : tLocal;
        }

        return tGlobal;
    }
}

std::unique_ptr<AnimationClip> AnimationRetargeter::retarget(const AnimationClip& pClip, const Skeleton& pSource, const Skeleton& pTarget)
{
    const auto& tTargetJoints = pTarget.getJoints();

    std::vector<int> tSourceIndex(tTargetJoints.size(), -1);

    for (size_t tIndex = 0; tIndex < tTargetJoints.size(); ++tIndex)
    {
        tSourceIndex[tIndex] = pSource.findJoint(tTargetJoints[tIndex].name);
    }

    std::vector<JointPose> tSourceBind = pSource.getBindPose();
    std::vector<JointPose> tTargetBind = pTarget.getBindPose();
    std::vector<glm::quat> tSourceBindGlobal = toGlobalRotations(pSource, tSourceBind);
    std::vector<glm::quat> tTargetBindGlobal = toGlobalRotations(pTarget, tTargetBind);

    int tSourceRoot = pSource.findJointByBaseName(kRootJoint);
    int tTargetRoot = pTarget.findJointByBaseName(kRootJoint);
    float tRootScale = 1.0f;

    if (tSourceRoot >= 0 && tTargetRoot >= 0)
    {
        float tSourceHeight = tSourceBind[static_cast<size_t>(tSourceRoot)].translation.y;
        float tTargetHeight = tTargetBind[static_cast<size_t>(tTargetRoot)].translation.y;
        tRootScale = std::abs(tSourceHeight) > 1e-4f ? tTargetHeight / tSourceHeight : 1.0f;
    }

    std::vector<AnimationClip::Track> tTracks(tTargetJoints.size());

    for (size_t tIndex = 0; tIndex < tTracks.size(); ++tIndex)
    {
        tTracks[tIndex].joint = static_cast<int>(tIndex);
    }

    std::vector<glm::mat4> tUnusedPalette;
    std::vector<glm::mat4> tSourceBindModel;
    std::vector<glm::mat4> tTargetBindModel;
    pSource.computePalette(tSourceBind, tUnusedPalette, tSourceBindModel);
    pTarget.computePalette(tTargetBind, tUnusedPalette, tTargetBindModel);

    glm::mat3 tSourceRootToModel(1.0f);
    float tModelScale = 1.0f;

    if (tSourceRoot >= 0 && tTargetRoot >= 0)
    {
        int tSourceParent = pSource.getJoints()[static_cast<size_t>(tSourceRoot)].parent;
        tSourceRootToModel = tSourceParent >= 0 ? glm::mat3(tSourceBindModel[static_cast<size_t>(tSourceParent)]) : glm::mat3(1.0f);

        float tSourceHeight = tSourceBindModel[static_cast<size_t>(tSourceRoot)][3].y;
        float tTargetHeight = tTargetBindModel[static_cast<size_t>(tTargetRoot)][3].y;
        tModelScale = std::abs(tSourceHeight) > 1e-4f ? tTargetHeight / tSourceHeight : 1.0f;
    }

    std::vector<AnimationClip::Key<glm::vec3>> tRootMotion;

    int tFrameCount = std::max(2, static_cast<int>(std::ceil(pClip.getDuration() * kSampleRate)) + 1);
    std::vector<JointPose> tSourcePose;
    std::vector<glm::quat> tTargetGlobal(tTargetJoints.size());

    for (int tFrame = 0; tFrame < tFrameCount; ++tFrame)
    {
        float tTime = std::min(static_cast<float>(tFrame) / kSampleRate, pClip.getDuration());

        tSourcePose = tSourceBind;
        pClip.sample(std::min(tTime, pClip.getDuration() * 0.999f), tSourcePose);
        std::vector<glm::quat> tSourceGlobal = toGlobalRotations(pSource, tSourcePose);

        if (tSourceRoot >= 0)
        {
            size_t tSourceSlot = static_cast<size_t>(tSourceRoot);
            glm::vec3 tShift = tSourceRootToModel * (tSourcePose[tSourceSlot].translation - tSourceBind[tSourceSlot].translation) * tModelScale;
            tRootMotion.push_back({ tTime, glm::vec3(tShift.x, 0.0f, tShift.z) });
        }

        for (size_t tIndex = 0; tIndex < tTargetJoints.size(); ++tIndex)
        {
            int tParent = tTargetJoints[tIndex].parent;
            glm::quat tParentGlobal = tParent >= 0 ? tTargetGlobal[static_cast<size_t>(tParent)] : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            int tSource = tSourceIndex[tIndex];

            if (tSource < 0)
            {
                tTargetGlobal[tIndex] = tParentGlobal * tTargetBind[tIndex].rotation;
            }
            else
            {
                size_t tSourceSlot = static_cast<size_t>(tSource);
                glm::quat tDelta = tSourceGlobal[tSourceSlot] * glm::inverse(tSourceBindGlobal[tSourceSlot]);
                tTargetGlobal[tIndex] = glm::normalize(tDelta * tTargetBindGlobal[tIndex]);
            }

            glm::quat tLocal = glm::normalize(glm::inverse(tParentGlobal) * tTargetGlobal[tIndex]);
            tTracks[tIndex].rotations.push_back({ tTime, tLocal });

            glm::vec3 tTranslation = tTargetBind[tIndex].translation;

            if (static_cast<int>(tIndex) == tTargetRoot && tSourceRoot >= 0)
            {
                size_t tSourceSlot = static_cast<size_t>(tSourceRoot);
                float tLift = tSourcePose[tSourceSlot].translation.y - tSourceBind[tSourceSlot].translation.y;
                tTranslation.y += tLift * tRootScale;
            }

            tTracks[tIndex].translations.push_back({ tTime, tTranslation });
        }
    }

    auto tClip = std::make_unique<AnimationClip>(pClip.getName(), pClip.getDuration(), std::move(tTracks));
    tClip->setRootMotion(std::move(tRootMotion));
    return tClip;
}
