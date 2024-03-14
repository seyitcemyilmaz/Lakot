#ifndef LAKOT_CAMERA_H
#define LAKOT_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define LAKOT_CAMERA_WORLD_UP				glm::vec3(0.0f, 1.0f, 0.0f)
#define LAKOT_CAMERA_FRONT					glm::vec3(0.0f, 0.0f, -1.0f)

#define LAKOT_DEFAULT_CAMERA_YAW			270.0
#define LAKOT_DEFAULT_CAMERA_PITCH			0.0

#define LAKOT_CAMERA_MAX_PITCH				89.9
#define LAKOT_CAMERA_MIN_PITCH				-89.9

class Camera
{
private:
    glm::vec3 mPosition;

    glm::vec3 mFrontVector;
    glm::vec3 mUpVector;
    glm::vec3 mRightVector;

    glm::vec3 mWorldUp;

    double mYaw;   // For x-axis
    double mPitch; // For y-axis
public:
    Camera(const glm::vec3& pPosition);

    glm::mat4 getViewMatrix() const;

    void updateCameraVectors();

    const glm::vec3& getPosition() const;

    const glm::vec3& getFrontVector() const;
    const glm::vec3& getUpVector() const;
    const glm::vec3& getRightVector() const;

    void setPosition(const glm::vec3& pPosition);

    void changePosition(const glm::vec3& pAmount);

    void changeYaw(double pAmount);
    void changePitch(double pAmount);
};

#endif
