#ifndef LAKOT_BONE_H
#define LAKOT_BONE_H

#include "resource/BoneResource.h"

#include "interface/IBone.h"

class Bone : public IBone
{
private:
    BoneResource* mBoneResource;

    glm::mat4 mTransformationMatrix;

public:
    Bone(BoneResource* pBoneResource, const glm::mat4& pTransformationMatrix);

    const std::string& getName() const override;

    const glm::mat4& getTransformationMatrix() const override;
    void setTransformationMatrix(const glm::mat4& pTransformationMatrix) override;

    BoneResource* getBoneResource();
};

#endif
