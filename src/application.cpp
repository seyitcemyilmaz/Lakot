#include "application.h"

#include "application/graphics/shader/shadermanager.h"
#include "application/graphics/camera/cameramanager.h"

Application::Application() {
    mRenderer = nullptr;
}

Application::~Application() {
    delete mRenderer;

    ShaderManager::getInstance()->deleteShaders();
    CameraManager::getInstance()->deleteCameras();
}

void Application::initialization() {
    mRenderer = new Renderer();

    initializeShaders();
    initializeCameras();
}

void Application::initializeShaders() {
    ShaderManager* tShaderManager = ShaderManager::getInstance();

    tShaderManager->addShader("shader", new Shader("vertex.shader", "fragment.shader"));
}

void Application::initializeCameras() {
    CameraManager* tCameraManager = CameraManager::getInstance();

    tCameraManager->addCamera("mainCamera", glm::vec3(2.0f));
}


void Application::processInputs() {

}

void Application::render() {
    mRenderer->render();
}
