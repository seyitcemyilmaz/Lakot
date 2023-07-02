#include "Window.h"

Window::Window(const char* pWindowTitle, int pWindowWidth, int pWindowHeight, glm::vec3 pWindowBackgroundColor) :
	mWindowTitle(pWindowTitle), mWindowWidth(pWindowWidth), mWindowHeight(pWindowHeight), mWindowBackgroundColor(pWindowBackgroundColor) {
	mIsWindowActive = false;
}

Window::~Window() { }

void Window::setWindowWidth(int pWindowWidth) {
	mWindowWidth = pWindowWidth;
}

void Window::setWindowHeight(int pWindowHeight) {
	mWindowHeight = pWindowHeight;
}

void Window::setIsWindowActive(bool pIsWindowActive) {
	mIsWindowActive = pIsWindowActive;
}

void Window::setWindowBackgroundColor(glm::vec3 pWindowBackgroundColor) {
	mWindowBackgroundColor = pWindowBackgroundColor;
}

int Window::getWindowWidth() {
	return mWindowWidth;
}

int Window::getWindowHeight() {
	return mWindowHeight;
}

bool Window::getIsWindowActive() {
	return mIsWindowActive;
}

glm::vec3 Window::getWindowBackgroundColor() {
	return mWindowBackgroundColor;
}
