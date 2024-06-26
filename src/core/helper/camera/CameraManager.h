#ifndef LAKOT_CAMERAMANAGER_H
#define LAKOT_CAMERAMANAGER_H

#include <map>
#include <string>

#include "Camera.h"

#define LAKOT_DEFAULT_CAMERA_MOVEMENT_SPEED			10.0

enum class CameraDirection
{
    eForward,
    eBackward,
    eLeft,
    eRight,
    eUp,
    eDown
};

class CameraManager
{
public:
    CameraManager();

    void setActiveCamera(const std::string& pName);
    Camera* getActiveCamera();
    std::string getActiveCameraName();

    void addCamera(const std::string& pName, const glm::vec3& pPosition);

    void deleteCameras();

    void updateActiveCameraPosition(CameraDirection pCameraDirection, double pDt);
    void updateActiveCameraDirection(double pDX, double pDY);
    void updateActiveCameraZoom(double pAmount);

private:
    std::map<std::string, Camera*> mCameras;
    std::pair<std::string, Camera*> mActiveCamera;

    // TODO: move to Camera class
    double mCameraMovementSpeed;

    void setActiveCameraNull();
};

#endif
