#include "WindowGLFW.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include "../api/GraphicsAPI.h"

using namespace lakot;

WindowGLFW::~WindowGLFW()
{

}

WindowGLFW::WindowGLFW()
    : Window()
    , mWindowObject(nullptr)
{

}

void WindowGLFW::initialize()
{
    spdlog::info("GLFW Window is initializing.");

    if (mInstance)
    {
        spdlog::error("Window cannot be initialize again.");
        return;
    }

    GraphicsAPI* tGraphicsAPI = GraphicsAPI::getInstance();

    if (!tGraphicsAPI)
    {
        throw "Graphics API is not found.";
    }

    if (mWindowObject)
    {
        throw "Window object should be null.";
    }

    int tGraphicsAPIVersionMajor = tGraphicsAPI->getVersionMajor();
    int tGraphicsAPIVersionMinor = tGraphicsAPI->getVersionMinor();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, tGraphicsAPIVersionMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, tGraphicsAPIVersionMinor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    mWindowObject = glfwCreateWindow(mWidth, mHeight, mTitle.c_str(), nullptr, nullptr);

    if (!mWindowObject)
    {
        throw "Window object is not created.";
    }

    spdlog::info("Window is created.");

    glfwMakeContextCurrent(mWindowObject);

    unsigned int tErrorCode = glewInit();

    if (tErrorCode != GLEW_OK)
    {
        throw "GLEW is not initialized.";
    }

    setCallbackFunctions();

    mInstance = this;

    mIsInitialized = true;

    spdlog::info("GLFW Window is initialized.");
}

void WindowGLFW::deinitialize()
{
    spdlog::info("GLFW Window is deinitializing.");

    // TODO: will be checked.

    glfwDestroyWindow(mWindowObject);
    mWindowObject = nullptr;

    mInstance = nullptr;

    spdlog::info("GLFW Window is deinitialized.");
}

void WindowGLFW::update()
{
    if (mCurrentTime != 0.0)
    {
        mCurrentTime = glfwGetTime();
        mTimeDifference = mCurrentTime - mPreviousTime;
        mPreviousTime = mCurrentTime;

        mFPSFrameCounter++;

        if (mCurrentTime - mFPSPreviousTime >= 1.0f)
        {
            mFPS = mFPSFrameCounter / (mCurrentTime - mFPSPreviousTime);

            mFPSFrameCounter = 0;
            mFPSPreviousTime = mCurrentTime;
        }
    }
    else
    {
        mCurrentTime = glfwGetTime();
        mTimeDifference = 1.0 / 60.0;
        mPreviousTime = mCurrentTime;

        mFPSFrameCounter = 1;
        mFPSPreviousTime = mCurrentTime;
        mFPS = 60.0;
    }

    glClearColor(0.6f, 0.2f, 0.7f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void WindowGLFW::nextFrame()
{
    glfwSwapBuffers(mWindowObject);
    glfwPollEvents();
}

void WindowGLFW::setCallbackFunctions()
{
    glfwSetWindowUserPointer(mWindowObject, this);

    glfwSetFramebufferSizeCallback(mWindowObject, WindowGLFW::frameBufferSizeCallback);
    glfwSetWindowCloseCallback(mWindowObject, WindowGLFW::windowCloseCallback);
    glfwSetWindowFocusCallback(mWindowObject, WindowGLFW::windowFocusCallback);
}

void WindowGLFW::frameBufferSizeCallback(GLFWwindow* pWindow, int pWidth, int pHeight)
{
    WindowGLFW* tWindowObject = static_cast<WindowGLFW*>(glfwGetWindowUserPointer(pWindow));

    tWindowObject->setWidth(pWidth);
    tWindowObject->setHeight(pHeight);

    if (tWindowObject->mOnResizeFunction)
    {
        tWindowObject->mOnResizeFunction(pWidth, pHeight);
    }

    spdlog::info("Window is resized. Width: {0}  Height: {1}", pWidth, pHeight);
}

void WindowGLFW::windowCloseCallback(GLFWwindow* pWindow)
{
    WindowGLFW* tWindowObject = static_cast<WindowGLFW*>(glfwGetWindowUserPointer(pWindow));

    tWindowObject->mIsActive = false;

    spdlog::info("Exit button is clicked.");
}

void WindowGLFW::windowFocusCallback(GLFWwindow* pWindow, int pFocused)
{
    WindowGLFW* tWindowObject = static_cast<WindowGLFW*>(glfwGetWindowUserPointer(pWindow));

    // if (pFocused)
    // {
    //     tWindowObject->setIsWindowFocused(true);
    //     spdlog::warn("Focused");
    // }
    // else
    // {
    //     tWindowObject->setIsWindowFocused(false);
    //     spdlog::warn("Not Focused");
    // }
}
