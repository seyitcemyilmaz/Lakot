#ifndef LAKOT_CAMERA_H
#define LAKOT_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define LAKOT_CAMERA_WORLD_UP           glm::vec3(0.0f, 1.0f, 0.0f)
#define LAKOT_CAMERA_FRONT              glm::vec3(0.0f, 0.0f, -1.0f)

#define LAKOT_DEFAULT_CAMERA_YAW        270.0f
#define LAKOT_DEFAULT_CAMERA_PITCH      0.0f

#define LAKOT_DEFAULT_CAMERA_SENSIVITY  1.0f
#define LAKOT_DEFAULT_CAMERA_ZOOM       45.0f

#define LAKOT_CAMERA_MIN_ZOOM           1.0f
#define LAKOT_CAMERA_MAX_ZOOM           45.0f

class Camera {
private:
    glm::vec3 mPosition;

    glm::vec3 mFrontVector;
    glm::vec3 mUpVector;
    glm::vec3 mRightVector;

    glm::vec3 mWorldUp;

    float mYaw;     // For x-axis
    float mPitch;   // For y-axis

    float mZoom;

    float mSensivity;

    void updateCameraVectors();
public:
    Camera(glm::vec3 pPosition);

    glm::vec3 getPosition();

    glm::vec3 getFrontVector();
    glm::vec3 getUpVector();
    glm::vec3 getRightVector();

    float getZoom();

    void changePosition(glm::vec3 pAmount);
    void changeZoom(float pAmount);
};

#endif
