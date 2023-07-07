#include "application.h"

#include "application/graphics/render/rendermanager.h"
#include "application/graphics/shader/shadermanager.h"

#include "core/resource/resourcemanager.h"
#include "core/helper/FileManager.h"
#include "core/helper/camera/cameramanager.h"

#include "PlatformFactory.h"

Application::Application() {
	mPlatform = nullptr;
	mGUI = nullptr;
}

Application::~Application() {
	ShaderManager::getInstance()->deleteShaders();
	CameraManager::getInstance()->deleteCameras();

	delete mPlatform;
	delete mGUI;
}

void Application::initialization() {
	mPlatform = PlatformFactory::createPlatformObject();

	mGUI = new GUI();

	initializeShaders();
	initializeCameras();
	initializeModels();

	ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "bear\\source\\bear.fbx"));

	glEnable(GL_DEPTH_TEST);
}

void Application::initializeShaders() {
	ShaderManager* tShaderManager = ShaderManager::getInstance();

	tShaderManager->addShader("shader", new Shader(FileManager::getInstance()->createPath(mPlatform->getAssetsPath(), "vertex.shader").c_str(),
												   FileManager::getInstance()->createPath(mPlatform->getAssetsPath(), "fragment.shader").c_str()));
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
	mPlatform->processInputs();
}

void Application::render() {
	// TODO: Add render function for scene
	RenderManager::getInstance()->renderScene();
	mModels[0]->draw();
	// TODO: Add render function for GUI
}

void Application::run() {
	std::function<void()> tRenderFunction = std::bind(&Application::render, this);
	mPlatform->run(tRenderFunction);
}

void Application::terminate() {
	mPlatform->getGraphicsAPI()->terminateGraphicsAPI();
}
