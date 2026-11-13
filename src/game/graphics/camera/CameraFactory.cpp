#include "CameraFactory.h"

#include "FPSCamera.h"

#include <SDL3/SDL.h>

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
        default:
        {
            SDL_Log("CameraFactory: unsupported camera type %d.", static_cast<int>(pCameraType));
        }
    }

    return tCamera;
}
