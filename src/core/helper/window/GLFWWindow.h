#ifndef LAKOT_GLFWWINDOW_H
#define LAKOT_GLFWWINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "helper/window/Window.h"

class GLFWWindow : public Window {
private:
	GLFWwindow* mWindowObject;

	void setCallbackFunctions();

	GLFWWindow(const char* pWindowTitle, int pWindowWidth, int pWindowHeight, glm::vec3 pWindowBackgroundColor);

protected:
	friend class WindowFactory;

public:
	~GLFWWindow();

	void initialize() override;
	void createWindow() override;
	void updateWindow() override;
	void destroyWindow() override;

	void* getWindowInstance() override;

	static void frameBufferSizeCallback(GLFWwindow* tWindow, int tWidth, int tHeight);
	static void windowCloseCallback(GLFWwindow* tWindow);
};

#endif
