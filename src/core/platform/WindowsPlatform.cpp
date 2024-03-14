#include "WindowsPlatform.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "graphicsAPI/OpenGLAPI.h"

#include "helper/window/WindowManager.h"

WindowsPlatform::WindowsPlatform()
    : Platform(new OpenGLAPI(LAKOT_GRAPHICS_API_VERSION_MAJOR, LAKOT_GRAPHICS_API_VERSION_MINOR))
{

}

std::string WindowsPlatform::getRootPath()
{
    std::filesystem::path tPlatformFilePath = __FILE__;
    std::filesystem::path tPlatformPath = tPlatformFilePath.parent_path();
    std::filesystem::path tCorePath = tPlatformPath.parent_path();
    std::filesystem::path tSrcPath = tCorePath.parent_path();
    std::filesystem::path tLakotRootPath = tSrcPath.parent_path();
    return tLakotRootPath.string();
}

void WindowsPlatform::run()
{
    WindowManager* tWindowManager = WindowManager::getInstance();

    while (tWindowManager->getIsWindowActive())
    {
        updateTimeDifference();
        mProcessInputsFunction();
        tWindowManager->updateWindow();
        mRenderFunction();
        glfwSwapBuffers(static_cast<GLFWwindow*>(tWindowManager->getWindow()->getWindowInstance()));
        glfwPollEvents();
    }
}

void WindowsPlatform::updateTimeDifference()
{
    if (mCurrentTime == 0.0)
    {
        mTimeDifference = 1.0 / 60.0;
        mCurrentTime = glfwGetTime();
        mPreviousTime = mCurrentTime;
    }
    else
    {
        mCurrentTime = glfwGetTime();
        mTimeDifference = mCurrentTime - mPreviousTime;
        mPreviousTime = mCurrentTime;
    }
}
