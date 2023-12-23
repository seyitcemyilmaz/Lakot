#include "Window.h"

Window::~Window()
{

}

Window::Window(const char* pWindowTitle, int pWindowWidth, int pWindowHeight, const glm::vec3& pWindowBackgroundColor)
    : mWindowTitle(pWindowTitle)
    , mWindowWidth(pWindowWidth)
    , mWindowHeight(pWindowHeight)
    , mIsWindowActive(false)
    , mIsWindowFocused(true)
    , mWindowBackgroundColor(pWindowBackgroundColor)
{

}

void Window::setWindowWidth(int pWindowWidth)
{
    mWindowWidth = pWindowWidth;
}

void Window::setWindowHeight(int pWindowHeight)
{
    mWindowHeight = pWindowHeight;
}

void Window::setIsWindowActive(bool pIsWindowActive)
{
    mIsWindowActive = pIsWindowActive;
}

void Window::setIsWindowFocused(bool pIsWindowFocused)
{
    mIsWindowFocused = pIsWindowFocused;
}

void Window::setWindowBackgroundColor(const glm::vec3& pWindowBackgroundColor)
{
    mWindowBackgroundColor = pWindowBackgroundColor;
}

int Window::getWindowWidth()
{
    return mWindowWidth;
}

int Window::getWindowHeight()
{
    return mWindowHeight;
}

bool Window::getIsWindowActive()
{
    return mIsWindowActive;
}

bool Window::getIsWindowFocused()
{
    return mIsWindowFocused;
}

const glm::vec3& Window::getWindowBackgroundColor()
{
    return mWindowBackgroundColor;
}
