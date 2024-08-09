#include "GraphicsAPIFactory.h"

#include "OpenGLAPI.h"

using namespace lakot;

GraphicsAPI* GraphicsAPIFactory::createGraphicsAPI()
{
#ifdef WIN32
    return new OpenGLAPI();
#else
#error Undefined OS
#endif
}
