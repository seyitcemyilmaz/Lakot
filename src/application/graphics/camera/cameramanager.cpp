#include "cameramanager.h"

CameraManager* CameraManager::mInstance = nullptr;

CameraManager::CameraManager() {
    setActiveCameraNull();
}

CameraManager* CameraManager::getInstance() {
    if (!mInstance) {
        mInstance = new CameraManager();
    }

    return mInstance;
}

void CameraManager::addCamera(std::string pName, glm::vec3 pPosition) {
    Camera* tCamera = new Camera(pPosition);

    if (mCameras.contains(pName)) {
        throw "Two cameras cannot have the same name";
    }

    mCameras[pName] = tCamera;

    Camera* tActiveCamera = mActiveCamera.second;

    if (!tActiveCamera) {
        mActiveCamera = std::make_pair(pName, tCamera);
    }
}

Camera* CameraManager::getActiveCamera() {
    Camera* tActiveCamera = mActiveCamera.second;
    return tActiveCamera;
}

std::string CameraManager::getActiveCameraName() {
    std::string tActiveCameraName = mActiveCamera.first;
    return tActiveCameraName;
}

void CameraManager::setActiveCamera(std::string pName) {
    if (!mCameras.contains(pName)) {
        throw "Invalid camera name.";
    }

    mActiveCamera = std::make_pair(pName, mCameras[pName]);
}

void CameraManager::setActiveCameraNull() {
    mActiveCamera = std::make_pair("", nullptr);
}

void CameraManager::deleteCameras() {
    for (auto tIterator = mCameras.begin(); tIterator != mCameras.end(); tIterator++) {
        Camera* tCamera = tIterator->second;
        delete tCamera;
    }

    mCameras.clear();

    setActiveCameraNull();
}

