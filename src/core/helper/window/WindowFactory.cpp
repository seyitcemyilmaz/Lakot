#include "WindowFactory.h"

#include "platform.h"

#include "GLFWWindow.h"

WindowFactory::WindowFactory() { }

Window* WindowFactory::createWindow() {
#ifdef LAKOT_PLATFORM_WINDOWS
	return createGLFWWindow();
#elif
#error "Not implemented."
#endif
}

Window* WindowFactory::createGLFWWindow() {
	return new GLFWWindow(LAKOT_DEFAULT_WINDOW_TITLE,
						  LAKOT_DEFAULT_GLFW_WINDOW_WIDTH,
						  LAKOT_DEFAULT_GLFW_WINDOW_HEIGHT,
						  LAKOT_DEFAULT_WINDOW_BACKGROUND);
}
