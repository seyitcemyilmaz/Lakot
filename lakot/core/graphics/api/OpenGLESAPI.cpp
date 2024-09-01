#include "OpenGLESAPI.h"

using namespace lakot;

OpenGLESAPI::~OpenGLESAPI()
{

}

OpenGLESAPI::OpenGLESAPI()
    : GraphicsAPI()
{
    mType = GraphicsAPIType::eOpenGLES;
    mVersionMajor = 3;
    mVersionMinor = 2;
}

void OpenGLESAPI::initialize()
{

}

void OpenGLESAPI::deinitialize()
{

}
