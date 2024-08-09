#ifndef WINDOWFACTORY_H
#define WINDOWFACTORY_H

#include <lakot/interface/graphics/api/IGraphicsAPI.h>

#include "Window.h"

namespace lakot {

class WindowFactory
{
public:
    virtual ~WindowFactory() = default;
    WindowFactory() = default;

    static Window* createWindow(GraphicsAPIType pGraphicsAPIType);
};

}

#endif
