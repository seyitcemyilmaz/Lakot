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

private:
    double mYaw;
    double mPitch;
};

}

#endif
