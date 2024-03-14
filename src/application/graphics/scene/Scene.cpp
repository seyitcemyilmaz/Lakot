#include "Scene.h"

#include "helper/controls/Keyboard.h"
#include "helper/controls/Mouse.h"

Scene::~Scene()
{
    mCameraManager->deleteCameras();
}

Scene::Scene()
{
    mCameraManager = new CameraManager();
    mCameraManager->addCamera("mainCamera", glm::vec3(0.0f, 0.0f, 2.0f));

    mProjection = new Projection();
}

void Scene::update(double pTimeDifference)
{
    for(unsigned int i = 0; i < mModels.size(); i++)
    {
        mModels[i]->calculateModelMatrix();
        mModels[i]->update(pTimeDifference);
    }
}

void Scene::processKeyboardInputs(double pTimeDifference)
{
    if (!Mouse::getInstance()->isButtonPressed(GLFW_MOUSE_BUTTON_MIDDLE))
    {
        return;
    }

    if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_W))
    {
        mCameraManager->updateActiveCameraPosition(CameraDirection::eForward, pTimeDifference);
    }

    if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_S))
    {
        mCameraManager->updateActiveCameraPosition(CameraDirection::eBackward, pTimeDifference);
    }

    if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_D))
    {
        mCameraManager->updateActiveCameraPosition(CameraDirection::eRight, pTimeDifference);
    }

    if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_A))
    {
        mCameraManager->updateActiveCameraPosition(CameraDirection::eLeft, pTimeDifference);
    }

    if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_SPACE))
    {
        mCameraManager->updateActiveCameraPosition(CameraDirection::eUp, pTimeDifference);
    }

    if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_LEFT_SHIFT))
    {
        mCameraManager->updateActiveCameraPosition(CameraDirection::eDown, pTimeDifference);
    }
}

void Scene::processMouseInputs(double pTimeDifference)
{
    if (!Mouse::getInstance()->isButtonPressed(GLFW_MOUSE_BUTTON_MIDDLE))
    {
        return;
    }

    double tDX = Mouse::getInstance()->getDX() * Mouse::getInstance()->getSensivity();
    double tDY = Mouse::getInstance()->getDY() * Mouse::getInstance()->getSensivity();

    if (tDX != 0.0f || tDY != 0.0f)
    {
        mCameraManager->updateActiveCameraDirection(tDX, tDY);
    }

    double tScrollDY = Mouse::getInstance()->getScrollDY();

    if (tScrollDY != 0.0f)
    {
        mProjection->changeZoom(tScrollDY);
    }
}

void Scene::addModel(Model* pModel)
{
    mModels.push_back(pModel);
}

bool Scene::deleteModel(Model* pModel)
{
    int tModelIndex = getModelIndex(pModel);

    if (tModelIndex == -1)
    {
        return false;
    }

    mModels.erase(mModels.begin() + tModelIndex);

    return true;
}

const std::vector<Model*>& Scene::getModels() const
{
    return mModels;
}

Projection* Scene::getProjection() const
{
    return mProjection;
}

Camera* Scene::getCamera() const
{
    return mCameraManager->getActiveCamera();
}

int Scene::getModelIndex(Model* pModel)
{
    for (unsigned int i = 0; i < mModels.size(); i++)
    {
        if (mModels[i] == pModel)
        {
            return i;
        }
    }

    return -1;
}
