#ifndef LAKOT_SKELETON_H
#define LAKOT_SKELETON_H

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace lakot
{

struct JointPose
{
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    glm::mat4 toMatrix() const;

    static JointPose blend(const JointPose& pFrom, const JointPose& pTo, float pWeight);
};

class Skeleton
{
public:
    struct Joint
    {
        std::string name;
        int parent{-1};
        JointPose bindPose;
        glm::mat4 inverseBind{1.0f};
    };

    // Parents must be added before their children.
    int addJoint(const std::string& pName, int pParent, const JointPose& pBindPose);
    void setInverseBind(int pJoint, const glm::mat4& pInverseBind);

    int findJoint(const std::string& pName) const;
    int findJointByBaseName(const std::string& pName) const;
    size_t getJointCount() const;
    const std::vector<Joint>& getJoints() const;

    std::vector<JointPose> getBindPose() const;
    const glm::mat4& getInverseBind(int pJoint) const;

    void computePalette(const std::vector<JointPose>& pLocalPoses, std::vector<glm::mat4>& pPalette, std::vector<glm::mat4>& pGlobals) const;

private:
    std::vector<Joint> mJoints;
};

}

#endif
