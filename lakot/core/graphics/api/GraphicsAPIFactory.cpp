#include "GraphicsAPIFactory.h"

#if defined(LAKOT_RENDERER_OPENGL)
#include "OpenGLAPI.h"
#endif

using namespace lakot;

GraphicsAPI* GraphicsAPIFactory::createGraphicsAPI()
{
#if defined(LAKOT_RENDERER_OPENGL)
    return new OpenGLAPI();
#else
#error "Undefined Graphics API"
#endif
}
