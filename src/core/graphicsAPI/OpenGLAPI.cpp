#include "OpenGLAPI.h"

OpenGLAPI::OpenGLAPI(GraphicsAPIMajor tGraphicsAPIMajor, GraphicsAPIMinor tGraphicsAPIMinor) :
	GraphicsAPI(GraphicsAPIType::eOpenGL, tGraphicsAPIMajor, tGraphicsAPIMinor) { }

OpenGLAPI::~OpenGLAPI() = default;

void OpenGLAPI::initializeGraphicsAPI() {
	std::cout << "OpenGL API is initalizing." << std::endl;

	if (!glfwInit()) {
		throw "GLFW is not initialized.";
	}

	std::cout << "OpenGL API is initialized." << std::endl;

	setIsGraphicsAPIInitialized(true);
}

void OpenGLAPI::terminateGraphicsAPI() {
	std::cout << "OpenGL API is terminating." << std::endl;
	if (getIsGraphicsAPIInitialized()) {
		glfwTerminate();
	}
	std::cout << "OpenGL API is terminated." << std::endl;
}

std::string OpenGLAPI::getGraphicsAPIString() {
	std::ostringstream tGraphicsAPIString;

	tGraphicsAPIString << "OpenGL " << getGraphicsAPIMajor() << "." << getGraphicsAPIMinor();
	return tGraphicsAPIString.str();
}
