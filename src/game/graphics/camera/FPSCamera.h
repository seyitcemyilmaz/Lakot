#ifndef LAKOT_FPSCAMERA_H
#define LAKOT_FPSCAMERA_H

#include "Camera.h"

namespace lakot
{

class FPSCamera final : public Camera
{
public:
    virtual ~FPSCamera() override;
    FPSCamera();

    void update() override;

    void processMouseMovement(double pXOffset, double pYOffset, bool pConstrainPitch = true);

    // Sets an absolute facing direction (degrees) - unlike
    // processMouseMovement, which only nudges the existing yaw. Used to
    // restore a saved/server-authoritative facing (login, portal travel).
    void setYaw(double pYaw);

private:
    double mYaw;
    double mPitch;
};

}

#endif
