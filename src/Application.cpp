#include "Application.h"

#include "core/resource/ResourceManager.h"
#include "core/helper/FileManager.h"
#include "core/helper/camera/CameraManager.h"
#include "core/helper/shader/ShaderManager.h"

#include "platform/PlatformFactory.h"

#include "application/graphics/model/ModelFactory.h"
#include "application/graphics/shader/ShaderModel.h"
#include "application/graphics/render/RenderManager.h"

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
    ShaderManager::getInstance()->addShader(
        new ShaderModel(
			ShaderName::eModelShader,
            FileManager::getInstance()->createPath(mPlatform->getAssetsPath(), "model.vsh").c_str(),
            FileManager::getInstance()->createPath(mPlatform->getAssetsPath(), "model.fsh").c_str())
        );
}

void Application::initializeCameras() {
	CameraManager* tCameraManager = CameraManager::getInstance();

	tCameraManager->addCamera("mainCamera", glm::vec3(0.0f, 0.0f, 2.0f));
}

void Application::initializeModels() {
	//ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "bear\\source\\bear.fbx"));
	//Model* tModel0 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "bear\\source\\bear.fbx"));
	//mModels.push_back(tModel0);

	//ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "test\\untitled.gltf"));
	//Model* tModel1 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "test\\untitled.gltf"));
	//mModels.push_back(tModel1);

	//ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "troll\\scene.gltf"));
	//Model* tModel2 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "troll\\scene.gltf"));
	//mModels.push_back(tModel2);

	ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "magnum\\source\\magnum.fbx"));
    Model* tModel3 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "magnum\\source\\magnum.fbx"));
    mModels.push_back(tModel3);

	tModel3->setPosition(0.0f, 0.0f, -40.0f);

	tModel3->changeRotationY(90);

	//ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "soldier\\scene.gltf"));
	//Model* tModel4 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "soldier\\scene.gltf"));
	//mModels.push_back(tModel4);

	//ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "axe\\source\\axe.fbx"));
	//Model* tModel5 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "axe\\source\\axe.fbx"));
	//mModels.push_back(tModel5);

	//ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "sword\\scene.gltf"));
	//Model* tModel6 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "sword\\scene.gltf"));
	//mModels.push_back(tModel6);
}

void Application::processInputs() {
	mPlatform->processInputs();
}

void Application::render() {
	// TODO: Add render function for scene
	RenderManager::getInstance()->renderScene();

	IShader* tShader = ShaderManager::getInstance()->getShader(ShaderName::eModelShader);

	for (int i = 0; i < mModels.size(); i++) {
		mModels[i]->calculateModelMatrix();
		const glm::mat4& tModelMatrix = mModels[i]->getModelMatrix();
		tShader->getShaderVariable(ShaderVariableName::eModel)->setMat4(tModelMatrix);
		RenderManager::getInstance()->renderModel(mModels[i], tShader);
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
