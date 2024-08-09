#ifndef LAKOT_ACAMERA_H
#define LAKOT_ACAMERA_H

#include <lakot/interface/graphics/camera/ICamera.h>

namespace lakot {

class ACamera : public ICamera
{
public:
    virtual ~ACamera() override;
    ACamera();

    CameraType getType() const override;

    const glm::vec3& getPosition() const override;
    void setPosition(const glm::vec3& pPosition) override;

    const glm::vec3& getFrontVector() const override;
    void setFrontVector(const glm::vec3& pFrontVector) override;

    const glm::vec3& getUpVector() const override;
    void setUpVector(const glm::vec3& pUpVector) override;

    const glm::vec3& getRightVector() const override;
    void setRightVector(const glm::vec3& pRightVector) override;

    const glm::vec3& getWorldUpVector() const override;
    void setWorldUpVector(const glm::vec3& pWorldUpVector) override;

    void changePosition(const glm::vec3& pAmount) override;

    const glm::mat4& getViewMatrix() const override;

protected:
    CameraType mType;

    glm::vec3 mPosition;

    glm::vec3 mFrontVector;
    glm::vec3 mUpVector;
    glm::vec3 mRightVector;

    glm::vec3 mWorldUpVector;

    glm::mat4 mViewMatrix;
};

}

#endif
