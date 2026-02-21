#ifndef LAKOT_CAMERAFACTORY_H
#define LAKOT_CAMERAFACTORY_H

#include <memory>

#include "Camera.h"

namespace lakot
{

class CameraFactory
{
public:
    static std::unique_ptr<Camera> createCamera(CameraType pCameraType);

private:
    ~CameraFactory() = default;
    CameraFactory() = default;
};

}

#endif
