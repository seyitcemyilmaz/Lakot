#ifndef LAKOT_CAMERA_H
#define LAKOT_CAMERA_H

#include <glm/glm.hpp>

#include "Type.h"

namespace lakot
{

class Camera
{
public:
    virtual ~Camera();
    explicit Camera(CameraType pType);

    virtual void update() = 0;
    void updateProjection(int pWidth, int pHeight);
    void calculateViewProjection();

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

    float getZoom() const;
    void setZoom(float pZoom);

    float getNear() const;
    void setNear(float pNear);

    float getFar() const;
    void setFar(float pFar);

    void changePosition(const glm::vec3& pAmount);

    const glm::mat4& getViewMatrix() const;
    const glm::mat4& getProjectionMatrix() const;
    const glm::mat4& getViewProjectionMatrix() const;

protected:
    CameraType mType;

    glm::vec3 mPosition;

    glm::vec3 mFrontVector;
    glm::vec3 mUpVector;
    glm::vec3 mRightVector;

    glm::vec3 mWorldUpVector;

    float mZoom;

    float mNear;
    float mFar;

    glm::mat4 mViewMatrix;
    glm::mat4 mProjectionMatrix;
    glm::mat4 mViewProjectionMatrix;
};

}

#endif
