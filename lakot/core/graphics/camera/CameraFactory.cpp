#include "CameraFactory.h"

#include <spdlog/spdlog.h>

#include "FPSCamera.h"

using namespace lakot;

Camera* CameraFactory::createCamera(CameraType pCameraType)
{
    Camera* tCamera = nullptr;

    switch (pCameraType)
    {
        case CameraType::eFPS:
        {
            tCamera = new FPSCamera();
            break;
        }
        case CameraType::eUndefined:
        {
            spdlog::warn("Camera type is undefined.");
            break;
        }
        default:
        {
            spdlog::error("Camera type is unknown.");
        }
    }

    return tCamera;
}
