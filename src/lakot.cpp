#include "lakot.h"

#include "core/platform.h"
#include "core/helper/windowmanager.h"

Lakot::Lakot() {
	mApplication = nullptr;
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
		glfwSwapBuffers((GLFWwindow*)tWindowManager->getWindow());
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
	mApplication->terminate();
}

Lakot::~Lakot() {
	delete mApplication;
}
