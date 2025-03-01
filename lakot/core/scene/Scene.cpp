#include "Scene.h"

#include <spdlog/spdlog.h>

#include <glm/gtc/matrix_transform.hpp>

#include "../graphics/window/Window.h"
#include "../graphics/camera/CameraFactory.h"
#include "../graphics/api/GraphicsAPI.h"

using namespace lakot;

Scene::~Scene()
{

}

Scene::Scene()
    : AScene()
    , mBox(nullptr)
{

}

void Scene::initialize()
{
    mActiveCamera = CameraFactory::createCamera(CameraType::eFPS);

    mBox = new Box();

    mBox->addBox(glm::vec3(0.0f), glm::vec3(2.0f));
    mBox->addBox(glm::vec3(0.0f), glm::vec3(3.0f));

    mBox->initialize();
}

void Scene::deinitialize()
{

}

void Scene::update()
{
    double tTimeDifference = Window::getInstance()->getTimeDifference();

    if (!mActiveCamera)
    {
        spdlog::warn("Active camera is not found.");
        return;
    }

    // mActiveCamera->setPosition(mActiveCamera->getPosition() - glm::vec3(0.0f, -tTimeDifference * 0.04, 0.0f));

    mActiveCamera->update();

    float tZoom = mActiveCamera->getZoom();

    float tNear = mActiveCamera->getNear();
    float tFar = mActiveCamera->getFar();

    float tWidth = Window::getInstance()->getWidth();
    float tHeight = Window::getInstance()->getHeight();

    float tAspectRatio = static_cast<float>(tWidth / tHeight);

    glm::mat4 tProjectionMatrix = glm::perspective(glm::radians(tZoom), tAspectRatio, tNear, tFar);
    glm::mat4 tViewMatrix = mActiveCamera->getViewMatrix();

    GraphicsAPI::getInstance()->getRenderer()->setViewMatrix(tViewMatrix);
    GraphicsAPI::getInstance()->getRenderer()->setProjectionMatrix(tProjectionMatrix);

    if (mBox)
    {
        GraphicsAPI::getInstance()->getRenderer()->render(mBox);
    }

    // Some updates.
}
