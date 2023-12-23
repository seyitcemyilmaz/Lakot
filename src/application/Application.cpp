#include "Application.h"

#include "core/resource/ResourceManager.h"
#include "core/helper/FileManager.h"
#include "core/helper/camera/CameraManager.h"
#include "core/helper/shader/ShaderManager.h"
#include "core/helper/window/WindowManager.h"

#include "platform/PlatformFactory.h"

#include "application/graphics/model/ModelFactory.h"
#include "application/graphics/shader/ShaderModel.h"
#include "application/graphics/render/RenderManager.h"

#include "helper/controls/Keyboard.h"
#include "helper/controls/Mouse.h"

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
	mGUI->initialize();

	initializeShaders();
	initializeCameras();
	initializeModels();	

	glEnable(GL_DEPTH_TEST);

	mPlatform->setRenderFunction(std::bind(&Application::render, this));
	mPlatform->setProcessInputFunction(std::bind(&Application::processInputs, this, std::placeholders::_1));
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
	ResourceManager::getInstance()->addModelResource(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "bmw\\scene.gltf"));
	Model* tModel0 = ModelFactory::createModel(FileManager::getInstance()->createPath(mPlatform->getModelsPath(), "bmw\\scene.gltf"));
	mModels.push_back(tModel0);
}

void Application::processInputs(double pTimeDifference) {
	if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_ESCAPE)) {
		WindowManager::getInstance()->closeWindow();
	}

	if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_W)) {
		CameraManager::getInstance()->updateActiveCameraPosition(CameraDirection::eForward, pTimeDifference);
	}

	if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_S)) {
		CameraManager::getInstance()->updateActiveCameraPosition(CameraDirection::eBackward, pTimeDifference);
	}

	if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_D)) {
		CameraManager::getInstance()->updateActiveCameraPosition(CameraDirection::eRight, pTimeDifference);
	}

	if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_A)) {
		CameraManager::getInstance()->updateActiveCameraPosition(CameraDirection::eLeft, pTimeDifference);
	}

	if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_SPACE)) {
		CameraManager::getInstance()->updateActiveCameraPosition(CameraDirection::eUp, pTimeDifference);
	}

	if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
		CameraManager::getInstance()->updateActiveCameraPosition(CameraDirection::eDown, pTimeDifference);
	}

	double tDX = Mouse::getInstance()->getDX() * Mouse::getInstance()->getSensivity();
	double tDY = Mouse::getInstance()->getDY() * Mouse::getInstance()->getSensivity();

	if (tDX != 0.0f || tDY != 0.0f) {
		CameraManager::getInstance()->updateActiveCameraDirection(tDX, tDY);
	}

	double tScrollDY = Mouse::getInstance()->getScrollDY();

	if (tScrollDY != 0.0f) {
		CameraManager::getInstance()->updateActiveCameraZoom(tScrollDY);
	}
}

void Application::render() {
	// TODO: Add render function for scene
	RenderManager::getInstance()->renderScene();

	double static tPreviousTime = glfwGetTime();
	double tCurrentTime = glfwGetTime();
	double tTimeDifference = tCurrentTime - tPreviousTime;
	tPreviousTime = tCurrentTime;

	IShader* tShader = ShaderManager::getInstance()->getShader(ShaderName::eModelShader);

	for (int i = 0; i < mModels.size(); i++) {
		mModels[i]->calculateModelMatrix();
		mModels[i]->update(tTimeDifference);
		const glm::mat4& tModelMatrix = mModels[i]->getModelMatrix();
		tShader->getShaderVariable(ShaderVariableName::eModel)->setMat4(tModelMatrix);
		RenderManager::getInstance()->renderModel(mModels[i], tShader);
	}

	mGUI->render();
}

void Application::run() {
	mPlatform->run();
}

void Application::terminate() {
	mPlatform->getGraphicsAPI()->terminateGraphicsAPI();
}
