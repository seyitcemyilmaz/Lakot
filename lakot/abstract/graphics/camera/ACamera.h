#ifndef LAKOT_ACAMERA_H
#define LAKOT_ACAMERA_H

#include <lakot/utilities/Object.h>
#include <lakot/utilities/Type.h>

#include <glm/glm.hpp>

namespace lakot {

class ACamera : public Object
{
public:
    virtual ~ACamera() override;
    ACamera();

    CameraType getType() const;

    const glm::vec3& getPosition() const;
    void setPosition(const glm::vec3& pPosition);

    const glm::vec3& getFrontVector() const;
    void setFrontVector(const glm::vec3& pFrontVector);

    const glm::vec3& getUpVector() const;
    void setUpVector(const glm::vec3& pUpVector);

    const glm::vec3& getRightVector() const;
    void setRightVector(const glm::vec3& pRightVector);

    const glm::vec3& getWorldUpVector() const;
    void setWorldUpVector(const glm::vec3& pWorldUpVector);

    void changePosition(const glm::vec3& pAmount);

    const glm::mat4& getViewMatrix() const;

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
