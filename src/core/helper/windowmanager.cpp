#include "windowmanager.h"

WindowManager* WindowManager::mInstance = nullptr;

WindowManager* WindowManager::getInstance() {
	if (!mInstance) {
		mInstance = new WindowManager(LAKOT_DEFAULT_WINDOW_TITLE,
									  LAKOT_DEFAULT_WINDOW_WIDTH,
									  LAKOT_DEFAULT_WINDOW_HEIGHT,
									  LAKOT_DEFAULT_WINDOW_BACKGROUND);
	}

	return mInstance;
}

WindowManager::WindowManager(const char* tWindowTitle, int tWindowWidth, int tWindowHeight, glm::vec3 tWindowBackgroundColor) :
	mWindowTitle(tWindowTitle),
	mWindowWidth(tWindowWidth),
	mWindowHeight(tWindowHeight),
	mWindowBackgroundColor(tWindowBackgroundColor) {
	mIsWindowActive = false;
}

void WindowManager::initializateWindow() {
	std::cout << "Window manager is inializating." << std::endl;

#if LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGL

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, LAKOT_GRAPHICS_API_VERSION_MAJOR);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, LAKOT_GRAPHICS_API_VERSION_MINOR);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	createWindow();

	if (!mWindow) {
		throw "Window is not created.";
	}

	std::cout << "Window is created." << std::endl;

	glfwMakeContextCurrent(mWindow);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		throw "GLAD is not initialized.";
	}

	std::cout << "GLAD is initialized." << std::endl;

#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGLES
#error Not implemented.
#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_NONE
#error Graphics API is not found.
#else
#error Undefined Graphics API.
#endif

	std::cout << "Window manager is initialized." << std::endl;
}

void WindowManager::updateWindow() {
	glClearColor(mWindowBackgroundColor.r, mWindowBackgroundColor.g, mWindowBackgroundColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void WindowManager::createWindow() {
	if (!mIsWindowActive) {
		mWindow = glfwCreateWindow(mWindowWidth, mWindowHeight, mWindowTitle, nullptr, nullptr);
		mIsWindowActive = true;
	}
}

void WindowManager::setIsWindowActive(bool tIsWindowActive) {
	mIsWindowActive = tIsWindowActive;
}

void WindowManager::setWindowWidth(int tWindowWidth) {
	mWindowWidth = tWindowWidth;
}

void WindowManager::setWindowHeight(int tWindowHeight) {
	mWindowHeight = tWindowHeight;
}

ContextWindow* WindowManager::getWindow() {
	return mWindow;
}

void WindowManager::setWindowBackgroundColor(glm::vec3 tWindowBackgroundColor) {
	mWindowBackgroundColor = tWindowBackgroundColor;
}

glm::vec3 WindowManager::getWindowBackgroundColor() {
	return mWindowBackgroundColor;
}

bool WindowManager::getIsWindowActive() {
	return mIsWindowActive;
}

int WindowManager::getWindowHeight() {
	return mWindowHeight;
}

int WindowManager::getWindowWidth() {
	return mWindowWidth;
}

void WindowManager::closeWindow() {
	setIsWindowActive(false);
}

void WindowManager::frameBufferSizeCallback(ContextWindow* tWindow, int tWidth, int tHeight) {
	WindowManager* tWindowObject = (WindowManager*)glfwGetWindowUserPointer(tWindow);

	tWindowObject->setWindowWidth(tWidth);
	tWindowObject->setWindowHeight(tHeight);

	glViewport(0, 0, tWidth, tHeight);

	std::cout << "Window is resized. Width: " << tWidth << " Height " << tHeight << std::endl;
}

void WindowManager::windowCloseCallback(ContextWindow* tWindow) {
	WindowManager* tWindowObject = (WindowManager*)glfwGetWindowUserPointer(tWindow);

	tWindowObject->setIsWindowActive(false);

	std::cout << "Exit button is clicked." << std::endl;
}
/*
WindowManager::~WindowManager() {
	#if LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGL
		glfwDestroyWindow(mWindow);
	#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_OPENGLES
		#error Not implemented.
	#elif LAKOT_GRAPHICS_API == LAKOT_GRAPHICS_API_NONE
		#error Graphics API is not found.
	#else
		#error Undefined Graphics API.
	#endif

}
*/
