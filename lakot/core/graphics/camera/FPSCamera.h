#ifndef LAKOT_FPSCAMERA_H
#define LAKOT_FPSCAMERA_H

#include "Camera.h"

namespace lakot {

class FPSCamera final : public Camera
{
public:
    virtual ~FPSCamera() override;
    FPSCamera();

    void initialize() override;
    void deinitialize() override;

    void update();

private:
    double mYaw;
    double mPitch;
};

}

#endif
