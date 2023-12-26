#include "WindowManager.h"

#include <spdlog/spdlog.h>

#include "helper/window/WindowFactory.h"

WindowManager* WindowManager::mInstance = nullptr;

WindowManager* WindowManager::getInstance()
{
    if (!mInstance)
    {
        mInstance = new WindowManager();
    }

    return mInstance;
}

WindowManager::WindowManager()
    : mActiveWindow(nullptr)
{

}

void WindowManager::initializateWindow()
{
    spdlog::info("Window manager is inializating.");

    mActiveWindow = WindowFactory::createWindow();

    mActiveWindow->initialize();

    spdlog::info("Window manager is initialized.");
}

void WindowManager::updateWindow()
{
    mActiveWindow->updateWindow();
}

void* WindowManager::getWindow()
{
    return mActiveWindow->getWindowInstance();
}

void WindowManager::setWindowBackgroundColor(glm::vec3 pWindowBackgroundColor)
{
    mActiveWindow->setWindowBackgroundColor(pWindowBackgroundColor);
}

glm::vec3 WindowManager::getWindowBackgroundColor()
{
    return mActiveWindow->getWindowBackgroundColor();
}

bool WindowManager::getIsWindowActive()
{
    return mActiveWindow->getIsWindowActive();
}

bool WindowManager::getIsWindowFocused()
{
    return mActiveWindow->getIsWindowFocused();
}

int WindowManager::getWindowHeight()
{
    return mActiveWindow->getWindowHeight();
}

int WindowManager::getWindowWidth()
{
    return mActiveWindow->getWindowWidth();
}

void WindowManager::closeWindow()
{
    mActiveWindow->setIsWindowActive(false);
}
