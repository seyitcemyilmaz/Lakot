#include "Window.h"

using namespace lakot;

Window* Window::mInstance = nullptr;

Window::~Window() {}

Window::Window()
    : AWindow()
{
    mWidth = 600;
    mHeight = 800;
}

Window* Window::getInstance()
{
    return mInstance;
}
