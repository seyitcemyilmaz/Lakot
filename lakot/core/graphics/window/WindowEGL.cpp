#include "WindowEGL.h"

#include <spdlog/spdlog.h>

using namespace lakot;

WindowEGL::~WindowEGL()
{

}

WindowEGL::WindowEGL()
    : Window()
{
    mTitle = "EGL Window";
}

void WindowEGL::initialize()
{
    spdlog::info("EGL Window is initializing.");


    spdlog::info("EGL Window is initialized.");
}

void WindowEGL::deinitialize()
{
    spdlog::info("EGL Window is deinitializing.");


    spdlog::info("EGL Window is deinitialized.");
}

void WindowEGL::update()
{

}

void WindowEGL::nextFrame()
{

}
