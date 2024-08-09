#ifndef LAKOT_ICAMERA_H
#define LAKOT_ICAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../../engine/IObject.h"

namespace lakot {

enum class CameraType
{
    eUndefined,
    eFPS
};

class ICamera : public IObject
{
public:
    virtual ~ICamera() = default;
    ICamera() = default;

    virtual void update() = 0;

    virtual CameraType getType() const = 0;

    virtual const glm::vec3& getPosition() const = 0;
    virtual void setPosition(const glm::vec3& pPosition) = 0;

    virtual const glm::vec3& getFrontVector() const = 0;
    virtual void setFrontVector(const glm::vec3& pFrontVector) = 0;

    virtual const glm::vec3& getUpVector() const = 0;
    virtual void setUpVector(const glm::vec3& pUpVector) = 0;

    virtual const glm::vec3& getRightVector() const = 0;
    virtual void setRightVector(const glm::vec3& pRightVector) = 0;

    virtual const glm::vec3& getWorldUpVector() const = 0;
    virtual void setWorldUpVector(const glm::vec3& pWorldUpVector) = 0;

    virtual void changePosition(const glm::vec3& pAmount) = 0;

    virtual const glm::mat4& getViewMatrix() const = 0;
};

}

#endif
