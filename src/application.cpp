#include "application.h"

#include "application/graphics/render/rendermanager.h"
#include "application/graphics/shader/shadermanager.h"
#include "application/graphics/camera/cameramanager.h"
#include "helper/controls/mouse.h"

Application::Application() {
    mGUI = nullptr;
}

Application::~Application() {
    ShaderManager::getInstance()->deleteShaders();
    CameraManager::getInstance()->deleteCameras();
}

void Application::initialization() {
    mGUI = new GUI();

    initializeShaders();
    initializeCameras();
    initializeModels();

    glEnable(GL_DEPTH_TEST);
}

void Application::initializeShaders() {
    ShaderManager* tShaderManager = ShaderManager::getInstance();

    tShaderManager->addShader("shader", new Shader("vertex.shader", "fragment.shader"));
}

void Application::initializeCameras() {
    CameraManager* tCameraManager = CameraManager::getInstance();

    tCameraManager->addCamera("mainCamera", glm::vec3(0.0f, 0.0f, 2.0f));
}

void Application::initializeModels() {
    Model* tModel = new Model();

    Mesh* tMesh = new Mesh();

    std::vector<float> tVertices {
        0.5f,  0.5f, 0.0f,  // top right
        0.5f, -0.5f, 0.0f,  // bottom right
       -0.5f, -0.5f, 0.0f,  // bottom left
       -0.5f,  0.5f, 0.0f   // top left
    };

    std::vector<unsigned int> tIndices {
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };

    tMesh->setVertices(tVertices);
    tMesh->setIndices(tIndices);

    tMesh->initializeBuffers();

    tModel->addMesh(tMesh);

    mModels.push_back(tModel);
}


void Application::processInputs() {
#if LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGL
    double tDX = Mouse::getInstance()->getDX() * Mouse::getInstance()->getSensivity();
    double tDY = Mouse::getInstance()->getDY() * Mouse::getInstance()->getSensivity();

    if (tDX != 0.0f || tDY != 0.0f) {
        CameraManager::getInstance()->updateActiveCameraDirection(tDX, tDY);
    }

    double tScrollDY = Mouse::getInstance()->getScrollDY();

    if (tScrollDY != 0.0f) {
        CameraManager::getInstance()->updateActiveCameraZoom(tScrollDY);
    }
#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGLES
    #error Not implemented.
#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_NONE
    #error Graphics API is not found.
#else
    #error Undefined Graphics API.
#endif
}

void Application::render() {
    // TODO: Add render function for scene
    RenderManager::getInstance()->renderScene();    
    mModels[0]->draw();
    // TODO: Add render functi on for GUI
}
