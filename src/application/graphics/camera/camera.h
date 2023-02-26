#ifndef LAKOT_CAMERA_H
#define LAKOT_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
private:
    glm::vec3 mPosition;

    glm::vec3 mFront;
    glm::vec3 mUp;
    glm::vec3 mRight;

    glm::vec3 mWorldUp;

    float mYaw;     // For x-axis
    float mPitch;   // For y-axis

    void updateCameraVectors();

public:
    Camera(glm::vec3 pPosition);

    void changePosition(glm::vec3 pAmount);
};

#endif
