#include "OpenGLAPI.h"

#include <spdlog/spdlog.h>

OpenGLAPI::~OpenGLAPI()
{

}

OpenGLAPI::OpenGLAPI(GraphicsAPIMajor tGraphicsAPIMajor,
                     GraphicsAPIMinor tGraphicsAPIMinor)
    : GraphicsAPI(GraphicsAPIType::eOpenGL, tGraphicsAPIMajor, tGraphicsAPIMinor)
{

}

void OpenGLAPI::initializeGraphicsAPI()
{
    spdlog::info("OpenGL API is initalizing.");

    if (!glfwInit())
    {
        throw "GLFW is not initialized.";
    }

    spdlog::info("OpenGL API is initialized.");

    setIsGraphicsAPIInitialized(true);
}

void OpenGLAPI::terminateGraphicsAPI()
{
    spdlog::info("OpenGL API is terminating.");

    if (getIsGraphicsAPIInitialized())
    {
        glfwTerminate();
    }

    spdlog::info("OpenGL API is terminated.");
}

std::string OpenGLAPI::getGraphicsAPIString()
{
    std::ostringstream tGraphicsAPIString;

    tGraphicsAPIString << "OpenGL " << getGraphicsAPIMajor() << "." << getGraphicsAPIMinor();
    return tGraphicsAPIString.str();
}
