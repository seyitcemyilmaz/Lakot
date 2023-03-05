#include "cameramanager.h"

CameraManager* CameraManager::mInstance = nullptr;

CameraManager* CameraManager::getInstance() {
    if (!mInstance) {
        mInstance = new CameraManager();
    }

    return mInstance;
}

CameraManager::CameraManager() {
    mCameraMovementSpeed = LAKOT_DEFAULT_CAMERA_MOVEMENT_SPEED;
    setActiveCameraNull();
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

void CameraManager::updateActiveCameraPosition(CameraDirection pCameraDirection, float pDt) {
    Camera* tActiveCamera = getActiveCamera();

    if (!tActiveCamera) {
        return;
    }

    glm::vec3 pAmount = glm::vec3(0.0f);

    if (pCameraDirection == CameraDirection::eForward) {
        pAmount = tActiveCamera->getFrontVector() * mCameraMovementSpeed * pDt;
    }
    else if (pCameraDirection == CameraDirection::eBackward) {
        pAmount = -(tActiveCamera->getFrontVector() * mCameraMovementSpeed * pDt);
    }
    else if (pCameraDirection == CameraDirection::eRight) {
        pAmount = tActiveCamera->getRightVector() * mCameraMovementSpeed * pDt;
    }
    else if (pCameraDirection == CameraDirection::eLeft) {
        pAmount = -(tActiveCamera->getRightVector() * mCameraMovementSpeed * pDt);
    }
    else if (pCameraDirection == CameraDirection::eUp) {
        pAmount = tActiveCamera->getUpVector() * mCameraMovementSpeed * pDt;
    }
    else if (pCameraDirection == CameraDirection::eDown) {
        pAmount = -(tActiveCamera->getUpVector() * mCameraMovementSpeed * pDt);
    }

    tActiveCamera->changePosition(pAmount);
}

void CameraManager::updateActiveCameraDirection(float pDX, float pDY) {
    Camera* tActiveCamera = getActiveCamera();

    if (!tActiveCamera) {
        return;
    }

    tActiveCamera->changeYaw(pDX);
    tActiveCamera->changePitch(pDY);

    tActiveCamera->updateCameraVectors();
}

void CameraManager::updateActiveCameraZoom(float pAmount) {
    Camera* tActiveCamera = getActiveCamera();

    if (!tActiveCamera) {
        return;
    }

    tActiveCamera->changeZoom(pAmount);
}
