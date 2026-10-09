#include "Skeleton.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace lakot;

glm::mat4 JointPose::toMatrix() const
{
    return glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
}

JointPose JointPose::blend(const JointPose& pFrom, const JointPose& pTo, float pWeight)
{
    JointPose tPose;
    tPose.translation = glm::mix(pFrom.translation, pTo.translation, pWeight);
    tPose.rotation = glm::slerp(pFrom.rotation, pTo.rotation, pWeight);
    tPose.scale = glm::mix(pFrom.scale, pTo.scale, pWeight);
    return tPose;
}

int Skeleton::addJoint(const std::string& pName, int pParent, const JointPose& pBindPose)
{
    mJoints.push_back({ pName, pParent, pBindPose, glm::mat4(1.0f) });
    return static_cast<int>(mJoints.size()) - 1;
}

void Skeleton::setInverseBind(int pJoint, const glm::mat4& pInverseBind)
{
    mJoints[static_cast<size_t>(pJoint)].inverseBind = pInverseBind;
}

int Skeleton::findJoint(const std::string& pName) const
{
    for (size_t tIndex = 0; tIndex < mJoints.size(); ++tIndex)
    {
        if (mJoints[tIndex].name == pName)
        {
            return static_cast<int>(tIndex);
        }
    }

    return -1;
}

int Skeleton::findJointByBaseName(const std::string& pName) const
{
    int tExact = findJoint(pName);

    if (tExact >= 0)
    {
        return tExact;
    }

    const std::string tSuffix = ":" + pName;

    for (size_t tIndex = 0; tIndex < mJoints.size(); ++tIndex)
    {
        const std::string& tName = mJoints[tIndex].name;

        if (tName.size() > tSuffix.size() && tName.compare(tName.size() - tSuffix.size(), tSuffix.size(), tSuffix) == 0)
        {
            return static_cast<int>(tIndex);
        }
    }

    return -1;
}

size_t Skeleton::getJointCount() const
{
    return mJoints.size();
}

const std::vector<Skeleton::Joint>& Skeleton::getJoints() const
{
    return mJoints;
}

std::vector<JointPose> Skeleton::getBindPose() const
{
    std::vector<JointPose> tPoses;
    tPoses.reserve(mJoints.size());

    for (const Joint& tJoint : mJoints)
    {
        tPoses.push_back(tJoint.bindPose);
    }

    return tPoses;
}

const glm::mat4& Skeleton::getInverseBind(int pJoint) const
{
    return mJoints[static_cast<size_t>(pJoint)].inverseBind;
}

void Skeleton::computePalette(const std::vector<JointPose>& pLocalPoses, std::vector<glm::mat4>& pPalette, std::vector<glm::mat4>& pGlobals) const
{
    std::vector<glm::mat4>& tGlobal = pGlobals;
    tGlobal.resize(mJoints.size());
    pPalette.resize(mJoints.size());

    for (size_t tIndex = 0; tIndex < mJoints.size(); ++tIndex)
    {
        const Joint& tJoint = mJoints[tIndex];
        glm::mat4 tLocal = pLocalPoses[tIndex].toMatrix();

        tGlobal[tIndex] = tJoint.parent >= 0 ? tGlobal[static_cast<size_t>(tJoint.parent)] * tLocal : tLocal;
        pPalette[tIndex] = tGlobal[tIndex] * tJoint.inverseBind;
    }
}
