#include "WindowFactory.h"

#include <spdlog/spdlog.h>

#if defined(LAKOT_WINDOW_GLFW)
#include "WindowGLFW.h"
#endif

#if defined(LAKOT_WINDOW_EGL)
#include "WindowEGL.h"
#endif

using namespace lakot;

Window* WindowFactory::createWindow()
{
#if defined(LAKOT_WINDOW_GLFW)
    return new WindowGLFW();
#elif defined(LAKOT_WINDOW_EGL)
    return new WindowEGL();
#else
#error "Undefined Window"
#endif
}
