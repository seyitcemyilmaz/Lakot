#include "RendererFactory.h"

#if defined(LAKOT_RENDERER_OPENGL)
#include "OpenGLRenderer.h"
#endif

#if defined(LAKOT_RENDERER_OPENGLES)
#include "OpenGLESRenderer.h"
#endif

#include "OpenGLRenderer.h"

using namespace lakot;

Renderer* RendererFactory::createRenderer()
{
#if defined(LAKOT_RENDERER_OPENGL)
    return new OpenGLRenderer();
#elif defined(LAKOT_RENDERER_OPENGLES)
    return new OpenGLESRenderer();
#else
#error "Undefined Renderer"
#endif
}
