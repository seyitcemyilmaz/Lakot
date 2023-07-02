#include "WindowsPlatform.h"

#include "graphicsAPI/openglapi.h"

#include "helper/windowmanager.h"
#include "helper/camera/cameramanager.h"

#include "helper/controls/keyboard.h"
#include "helper/controls/mouse.h"

WindowsPlatform::WindowsPlatform() : Platform(new OpenGLAPI(LAKOT_GRAPHICS_API_VERSION_MAJOR, LAKOT_GRAPHICS_API_VERSION_MINOR)) { }

std::string WindowsPlatform::getRootPath() {
	std::filesystem::path tPlatformFilePath = __FILE__;
	std::filesystem::path tCorePath = tPlatformFilePath.parent_path();
	std::filesystem::path tSrcPath = tCorePath.parent_path();
	std::filesystem::path tLakotRootPath = tSrcPath.parent_path();
	return tLakotRootPath.string();
}

void WindowsPlatform::processInputs(double* pPreviousTime) {
	double tCurrentTime = glfwGetTime();

	double tDt = tCurrentTime - *pPreviousTime;

	processMouseInputs();
	processKeyboardInputs(tDt);

	*pPreviousTime = tCurrentTime;
}

void WindowsPlatform::processKeyboardInputs(double pDt) {
	if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_ESCAPE)) {
		WindowManager::getInstance()->closeWindow();
	}

	if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_W)) {
		CameraManager::getInstance()->updateActiveCameraPosition(CameraDirection::eForward, pDt);
	}

	if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_S)) {
		CameraManager::getInstance()->updateActiveCameraPosition(CameraDirection::eBackward, pDt);
	}

	if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_D)) {
		CameraManager::getInstance()->updateActiveCameraPosition(CameraDirection::eRight, pDt);
	}

	if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_A)) {
		CameraManager::getInstance()->updateActiveCameraPosition(CameraDirection::eLeft, pDt);
	}

	if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_SPACE)) {
		CameraManager::getInstance()->updateActiveCameraPosition(CameraDirection::eUp, pDt);
	}

	if (Keyboard::getInstance()->isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
		CameraManager::getInstance()->updateActiveCameraPosition(CameraDirection::eDown, pDt);
	}
}

void WindowsPlatform::processMouseInputs() {
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
