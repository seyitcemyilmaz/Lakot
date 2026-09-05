#ifndef LAKOT_THIRDPERSONCAMERA_H
#define LAKOT_THIRDPERSONCAMERA_H

#include "Camera.h"

namespace lakot
{

// Classic MMORPG follow camera: orbits a target position (the character) at
// a distance instead of being the player itself. The character is modeled
// as always facing the direction the camera looks (no independent facing
// state) - simplest classic-camera behavior.
class ThirdPersonCamera final : public Camera
{
public:
    virtual ~ThirdPersonCamera() override;
    ThirdPersonCamera();

    void update() override;

    // Only call while right-click is held - see WorldScene::handleEvent.
    void orbit(double pXOffset, double pYOffset);

    // Scroll-wheel distance adjustment, clamped.
    void zoom(float pAmount);

    void setTargetPosition(const glm::vec3& pPosition);
    const glm::vec3& getTargetPosition() const;

    // WASD moves this, not the base class's mPosition (the eye) - the eye
    // is derived from the target every update().
    void moveTarget(const glm::vec3& pAmount);

    // Sets an absolute facing direction (degrees) - restores a
    // saved/server-authoritative facing (login, portal travel).
    void setYaw(double pYaw);

private:
    glm::vec3 mTargetPosition;

    double mYaw;
    double mPitch;
    float mDistance;
};

}

#endif
