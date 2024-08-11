#include "RendererFactory.h"

#include "OpenGLRenderer.h"

using namespace lakot;

Renderer* RendererFactory::createRenderer(GraphicsAPIType pGraphicsAPIType)
{
    if (pGraphicsAPIType == GraphicsAPIType::eOpenGL)
    {
        return new OpenGLRenderer();
    }

    throw "Undefined Renderer.";
}
