#include "CameraFactory.h"

#include "FPSCamera.h"

using namespace lakot;

std::unique_ptr<Camera> CameraFactory::createCamera(CameraType pCameraType)
{
    std::unique_ptr<Camera> tCamera = nullptr;

    switch (pCameraType)
    {
        case CameraType::eFPS:
        {
            tCamera = std::make_unique<FPSCamera>();
            break;
        }
        case CameraType::eUndefined:
        {
            // TODO: add message
            break;
        }
        default:
        {
            // TODO: add message
        }
    }

    return tCamera;
}
