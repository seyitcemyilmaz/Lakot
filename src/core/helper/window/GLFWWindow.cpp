#include "GLFWWindow.h"

#include <spdlog/spdlog.h>

#include "helper/controls/Keyboard.h"
#include "helper/controls/Mouse.h"

#include "platform/Platform.h"

GLFWWindow::~GLFWWindow()
{

}

GLFWWindow::GLFWWindow(const char* pWindowTitle,
                       int pWindowWidth,
                       int pWindowHeight,
                       const glm::vec3& pWindowBackgroundColor)
    : Window(pWindowTitle, pWindowWidth, pWindowHeight, pWindowBackgroundColor)
{

}

void GLFWWindow::initialize()
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, LAKOT_GRAPHICS_API_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, LAKOT_GRAPHICS_API_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    createWindow();

    if (!mWindowObject)
    {
        throw "Window is not created.";
    }

    spdlog::info("Window is created.");

    glfwMakeContextCurrent(mWindowObject);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        throw "GLAD is not initialized.";
    }

    spdlog::info("GLAD is initialized.");

    setCallbackFunctions();

    glfwFocusWindow(mWindowObject);
}

void GLFWWindow::createWindow()
{
    if (!mIsWindowActive)
    {
        mWindowObject = glfwCreateWindow(mWindowWidth, mWindowHeight, mWindowTitle, nullptr, nullptr);
        mIsWindowActive = true;
    }
}

void GLFWWindow::updateWindow()
{
    glClearColor(mWindowBackgroundColor.r,
                 mWindowBackgroundColor.g,
                 mWindowBackgroundColor.b,
                 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLFWWindow::destroyWindow()
{
    glfwDestroyWindow(mWindowObject);
}

void GLFWWindow::setCallbackFunctions()
{
    glfwSetWindowUserPointer(mWindowObject, this);

    glfwSetFramebufferSizeCallback(mWindowObject, GLFWWindow::frameBufferSizeCallback);
    glfwSetWindowCloseCallback(mWindowObject, GLFWWindow::windowCloseCallback);
    glfwSetWindowFocusCallback(mWindowObject, GLFWWindow::windowFocusCallback);

    glfwSetKeyCallback(mWindowObject, Keyboard::keyCallback);

    glfwSetScrollCallback(mWindowObject, Mouse::scrollCallback);
    glfwSetCursorPosCallback(mWindowObject, Mouse::cursorPositionCallBack);
    glfwSetCursorEnterCallback(mWindowObject, Mouse::cursorEnterCallback);
    glfwSetMouseButtonCallback(mWindowObject, Mouse::mouseButtonCallback);
}

void* GLFWWindow::getWindowInstance()
{
    return mWindowObject;
}

void GLFWWindow::frameBufferSizeCallback(GLFWwindow* pWindow, int pWidth, int pHeight)
{
    GLFWWindow* tWindowObject = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(pWindow));

    tWindowObject->setWindowWidth(pWidth);
    tWindowObject->setWindowHeight(pHeight);

    // TODO: Check !!!
    if (tWindowObject->mOnResizeFunction)
    {
        tWindowObject->mOnResizeFunction(tWindowObject, pWidth, pHeight);
    }

    spdlog::info("Window is resized. Width: {0}  Height: {1}", pWidth, pHeight);
}

void GLFWWindow::windowCloseCallback(GLFWwindow* pWindow)
{
    GLFWWindow* tWindowObject = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(pWindow));

    tWindowObject->setIsWindowActive(false);

    spdlog::info("Exit button is clicked.");
}

void GLFWWindow::windowFocusCallback(GLFWwindow* pWindow, int pFocused)
{
    GLFWWindow* tWindowObject = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(pWindow));

    if (pFocused)
    {
        tWindowObject->setIsWindowFocused(true);
        spdlog::warn("Focused");
    }
    else
    {
        tWindowObject->setIsWindowFocused(false);
        spdlog::warn("Not Focused");
    }
}
