#include "camera.h"

Camera::Camera(glm::vec3 pPosition) {
    mPosition = pPosition;
}

void Camera::changePosition(glm::vec3 pAmount) {
    mPosition += pAmount;
}

void Camera::updateCameraVectors() {
    glm::vec3 tNewFront(cos(glm::radians(mYaw)) * cos(glm::radians(mPitch)),
                        sin(glm::radians(mPitch)),
                        sin(glm::radians(mYaw)) * cos(glm::radians(mPitch))
                        );

    mFront = glm::normalize(tNewFront);
    mRight = glm::normalize(glm::cross(mFront, mWorldUp));
    mUp = glm::normalize(glm::cross(mRight, mFront));
}
