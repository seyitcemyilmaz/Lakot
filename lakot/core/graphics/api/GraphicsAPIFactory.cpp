#include "GraphicsAPIFactory.h"

#if defined(LAKOT_RENDERER_OPENGL)
#include "OpenGLAPI.h"
#endif

#if defined(LAKOT_RENDERER_OPENGLES)
#include "OpenGLESAPI.h"
#endif

using namespace lakot;

GraphicsAPI* GraphicsAPIFactory::createGraphicsAPI()
{
#if defined(LAKOT_RENDERER_OPENGL)
    return new OpenGLAPI();
#elif defined(LAKOT_RENDERER_OPENGLES)
    return new OpenGLESAPI();
#else
#error "Undefined Graphics API"
#endif
}
