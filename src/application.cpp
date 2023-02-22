#include "application.h"

Application::Application() {
    mRenderer = nullptr;
    mShaderManager = nullptr;
}

Application::~Application() {
    delete mRenderer;
    delete mShaderManager;
}

void Application::initialization() {
    mRenderer = new Renderer();
    mShaderManager = new ShaderManager();

    initializeShaders();
}

void Application::initializeShaders() {
    mShaderManager->addShader("shader", new Shader("vertex.shader", "fragment.shader"));
}

void Application::processInputs() {

}

void Application::render() {
    mRenderer->render();
}
