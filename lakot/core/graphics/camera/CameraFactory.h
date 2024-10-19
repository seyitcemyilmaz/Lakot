#ifndef LAKOT_CAMERAFACTORY_H
#define LAKOT_CAMERAFACTORY_H

#include "Camera.h"

namespace lakot {

class CameraFactory
{
public:
    static Camera* createCamera(CameraType pCameraType);

private:
    ~CameraFactory() = default;
    CameraFactory() = default;
};

}

#endif
