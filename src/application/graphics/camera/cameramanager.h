#ifndef LAKOT_CAMERAMANAGER_H
#define LAKOT_CAMERAMANAGER_H

#include "platform.h"

#include "camera.h"

#define LAKOT_DEFAULT_CAMERA_MOVEMENT_SPEED      10.0f

enum CameraDirection {
    eForward,
    eBackward,
    eLeft,
    eRight,
    eUp,
    eDown
};

class CameraManager {
private:
    static CameraManager* mInstance;

    std::map<std::string, Camera*> mCameras;
    std::pair<std::string, Camera*> mActiveCamera;

    float mCameraMovementSpeed;

    void setActiveCameraNull();

    CameraManager();
public:
    static CameraManager* getInstance();

    void setActiveCamera(std::string pName);
    Camera* getActiveCamera();
    std::string getActiveCameraName();

    void addCamera(std::string pName, glm::vec3 pPosition);

    void deleteCameras();

    void changeActiveCameraPosition(CameraDirection pCameraDirection, float pDt);
};

#endif
