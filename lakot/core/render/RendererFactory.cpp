#include "RendererFactory.h"

#if defined(LAKOT_RENDERER_OPENGL)
#include "OpenGLRenderer.h"
#endif

#include "OpenGLRenderer.h"

using namespace lakot;

ARenderer* RendererFactory::createRenderer()
{
#if defined(LAKOT_RENDERER_OPENGL)
    return new OpenGLRenderer();
#else
#error "Undefined Renderer"
#endif
}
