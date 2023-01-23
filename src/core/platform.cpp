#include "platform.h"

Platform::Platform() {
    #if LAKOT_PLATFORM == LAKOT_PLATFORM_WINDOWS
        mGraphicsAPI = new OpenGLAPI(LAKOT_GRAPHICS_API_VERSION_MAJOR, LAKOT_GRAPHICS_API_VERSION_MINOR);
    #elif LAKOT_PLATFORM == LAKOT_PLATFORM_LINUX
        mGraphicsAPI = new OpenGLAPI(LAKOT_GRAPHICS_API_VERSION_MAJOR, LAKOT_GRAPHICS_API_VERSION_MINOR);
    #elif LAKOT_PLATFORM == LAKOT_PLATFORM_ANDROID
        throw "Not implemented.";
    #else
        throw "Unknown platform.";
    #endif
}

GraphicsAPI* Platform::getGraphicsAPI() {
    return mGraphicsAPI;
}

Platform::~Platform() {
    delete mGraphicsAPI;
}
