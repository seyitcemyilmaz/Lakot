#ifndef LAKOT_WINDOWFACTORY_H
#define LAKOT_WINDOWFACTORY_H

#include "Window.h"

#define LAKOT_DEFAULT_WINDOW_TITLE				"LAKOT"
#define LAKOT_DEFAULT_WINDOW_BACKGROUND			glm::vec3(0.3f, 0.2f, 0.5f)

#define LAKOT_DEFAULT_GLFW_WINDOW_WIDTH			800
#define LAKOT_DEFAULT_GLFW_WINDOW_HEIGHT		800

class WindowFactory
{
private:
	WindowFactory();

	static Window* createGLFWWindow();
public:
	static Window* createWindow();
};

#endif
