#include "Scene.h"

#include "../graphics/window/Window.h"
#include "../graphics/camera/CameraFactory.h"

using namespace lakot;

Scene::~Scene()
{

}

Scene::Scene()
    : AScene()
{

}

void Scene::initialize()
{
    mActiveCamera = CameraFactory::createCamera(CameraType::eFPS);
}

void Scene::deinitialize()
{

}

void Scene::update()
{
    double tTimeDifference = Window::getInstance()->getTimeDifference();

    // Some updates.
}
