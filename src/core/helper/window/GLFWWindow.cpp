#include "GLFWWindow.h"

#include <spdlog/spdlog.h>

#include "helper/controls/Keyboard.h"
#include "helper/controls/Mouse.h"

#include "platform/Platform.h"

GLFWWindow::GLFWWindow(const char* pWindowTitle, int pWindowWidth, int pWindowHeight, glm::vec3 pWindowBackgroundColor)
	: Window(pWindowTitle, pWindowWidth, pWindowHeight, pWindowBackgroundColor) { }

GLFWWindow::~GLFWWindow() {

}

void GLFWWindow::initialize() {
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, LAKOT_GRAPHICS_API_VERSION_MAJOR);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, LAKOT_GRAPHICS_API_VERSION_MINOR);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	createWindow();

	if (!mWindowObject) {
		throw "Window is not created.";
	}

	spdlog::info("Window is created.");

	glfwMakeContextCurrent(mWindowObject);

	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
		throw "GLAD is not initialized.";
	}

	spdlog::info("GLAD is initialized.");

	setCallbackFunctions();
}

void GLFWWindow::createWindow() {
	if (!mIsWindowActive) {
		mWindowObject = glfwCreateWindow(mWindowWidth, mWindowHeight, mWindowTitle, nullptr, nullptr);
		mIsWindowActive = true;
	}
}

void GLFWWindow::updateWindow() {
	glClearColor(mWindowBackgroundColor.r, mWindowBackgroundColor.g, mWindowBackgroundColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLFWWindow::destroyWindow() {
	glfwDestroyWindow(mWindowObject);
}

void GLFWWindow::setCallbackFunctions() {
	glfwSetWindowUserPointer(mWindowObject, this);
	glfwSetFramebufferSizeCallback(mWindowObject, GLFWWindow::frameBufferSizeCallback);
	glfwSetWindowCloseCallback(mWindowObject, GLFWWindow::windowCloseCallback);
	glfwSetKeyCallback(mWindowObject, Keyboard::keyCallback);
	glfwSetMouseButtonCallback(mWindowObject, Mouse::mouseButtonCallback);
	glfwSetScrollCallback(mWindowObject, Mouse::scrollCallback);
	glfwSetCursorPosCallback(mWindowObject, Mouse::cursorPositionCallBack);
}

void* GLFWWindow::getWindowInstance() {
	return mWindowObject;
}

void GLFWWindow::frameBufferSizeCallback(GLFWwindow* tWindow, int tWidth, int tHeight) {
	GLFWWindow* tWindowObject = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(tWindow));

	tWindowObject->setWindowWidth(tWidth);
	tWindowObject->setWindowHeight(tHeight);

	glViewport(0, 0, tWidth, tHeight);

	spdlog::info("Window is resized. Width: {0}  Height: {1}", tWidth, tHeight);
}

void GLFWWindow::windowCloseCallback(GLFWwindow* tWindow) {
	GLFWWindow* tWindowObject = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(tWindow));

	tWindowObject->setIsWindowActive(false);

	spdlog::info("Exit button is clicked.");
}
