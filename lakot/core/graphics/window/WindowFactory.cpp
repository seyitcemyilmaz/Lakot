#include "WindowFactory.h"

#include "WindowGLFW.h"

#include <spdlog/spdlog.h>

using namespace lakot;

Window* WindowFactory::createWindow(GraphicsAPIType pGraphicsAPIType)
{
    if (pGraphicsAPIType == GraphicsAPIType::eUndefined)
    {
        spdlog::error("Graphics API type is undefined.");
        return nullptr;
    }

    if (pGraphicsAPIType == GraphicsAPIType::eOpenGL)
    {
        return new WindowGLFW();
    }

    throw "Graphics API type is undefined.";
}
