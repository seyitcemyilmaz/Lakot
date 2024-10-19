#ifndef WINDOWFACTORY_H
#define WINDOWFACTORY_H

#include "Window.h"

namespace lakot {

class WindowFactory
{
public:
    virtual ~WindowFactory() = default;
    WindowFactory() = default;

    static Window* createWindow();
};

}

#endif
