#include "lakot.h"

#include "helper/controls/keyboard.h"
#include "helper/controls/mouse.h"

Lakot::Lakot() {
	mPlatform = nullptr;
	mApplication = nullptr;
}

void Lakot::initializeLakot() {
	mPlatform = new Platform();
	mPlatform->getGraphicsAPI()->initializeGraphicsAPI();

	if (!mPlatform->getGraphicsAPI()->getIsGraphicsAPIInitialized()) {
		throw "Graphics API is not initialized.";
	}

	std::cout << "Lakot Graphics API: " << mPlatform->getGraphicsAPI()->getGraphicsAPIString() << std::endl;

	WindowManager::getInstance()->initializateWindow();

	initializeCallbackFunctions();
}

void Lakot::initializeCallbackFunctions() {
#if LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGL
	WindowManager* tWindowManager = WindowManager::getInstance();

	glfwSetWindowUserPointer(tWindowManager->getWindow(), tWindowManager);
	glfwSetFramebufferSizeCallback(tWindowManager->getWindow(), WindowManager::frameBufferSizeCallback);
	glfwSetWindowCloseCallback(tWindowManager->getWindow(), WindowManager::windowCloseCallback);
	glfwSetKeyCallback(tWindowManager->getWindow(), Keyboard::keyCallback);
	glfwSetMouseButtonCallback(tWindowManager->getWindow(), Mouse::mouseButtonCallback);
	glfwSetScrollCallback(tWindowManager->getWindow(), Mouse::scrollCallback);
	glfwSetCursorPosCallback(tWindowManager->getWindow(), Mouse::cursorPositionCallBack);
#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGLES
#error Not implemented.
#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_NONE
#error Graphics API is not found.
#else
#error Undefined Graphics API.
#endif
}

void Lakot::initalizeApplication() {
	mApplication = new Application();
	mApplication->initialization();
}

void Lakot::runApplication() {
	std::cout << "Application is running." << std::endl;
#if LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGL
	WindowManager* tWindowManager = WindowManager::getInstance();

	while (tWindowManager->getIsWindowActive()) {
		mApplication->processInputs();
		tWindowManager->updateWindow();
		mApplication->render();
		glfwSwapBuffers(tWindowManager->getWindow());
		glfwPollEvents();
	}
#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGLES
#error Not implemented.
#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_NONE
#error Graphics API is not found.
#else
#error Undefined Graphics API.
#endif
	std::cout << "Application is stopped." << std::endl;
}

void Lakot::terminateApplication() {
	mPlatform->getGraphicsAPI()->terminateGraphicsAPI();
}

Lakot::~Lakot() {
	delete mPlatform;
}
