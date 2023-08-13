#include "Application.h"

#include "application/graphics/render/RenderManager.h"
#include "application/graphics/shader/ShaderManager.h"

#include "core/resource/ResourceManager.h"
#include "core/helper/FileManager.h"
#include "core/helper/camera/CameraManager.h"

#include "platform/PlatformFactory.h"

#include "application/graphics/model/ModelFactory.h"

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
    ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "bear\\source\\bear.fbx"));
    Model* tModel0 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "bear\\source\\bear.fbx"));
    mModels.push_back(tModel0);

    ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "test\\untitled.gltf"));
    Model* tModel1 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "test\\untitled.gltf"));
    mModels.push_back(tModel1);

    ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "troll\\scene.gltf"));
    Model* tModel2 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "troll\\scene.gltf"));
    mModels.push_back(tModel2);

    ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "magnum\\source\\magnum.fbx"));
    Model* tModel3 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "magnum\\source\\magnum.fbx"));
    mModels.push_back(tModel3);

    ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "soldier\\scene.gltf"));
    Model* tModel4 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "soldier\\scene.gltf"));
    mModels.push_back(tModel4);

    ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "axe\\source\\axe.fbx"));
    Model* tModel5 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "axe\\source\\axe.fbx"));
    mModels.push_back(tModel5);

    ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "sword\\scene.gltf"));
    Model* tModel6 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "sword\\scene.gltf"));
    mModels.push_back(tModel6);
}

void Application::processInputs() {
	mPlatform->processInputs();
}

void Application::render() {
	// TODO: Add render function for scene
	RenderManager::getInstance()->renderScene();
	for (int i = 0; i < mModels.size(); i++) {
		mModels[i]->draw();
	}
	// TODO: Add render function for GUI
}

void Application::run() {
	std::function<void()> tRenderFunction = std::bind(&Application::render, this);
	mPlatform->run(tRenderFunction);
}

void Application::terminate() {
	mPlatform->getGraphicsAPI()->terminateGraphicsAPI();
}
